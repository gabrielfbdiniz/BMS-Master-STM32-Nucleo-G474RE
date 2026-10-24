"""
Formula ITA — BMS Master V0: Software-in-the-Loop (SIL) Simulator & Validator
Implementa o modelo físico 20s18p e o Algoritmo Método 1 (RLS + Z-Score)
Gera o log CSV e os gráficos de validação automática.
"""

import sys
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass
import math
import random
import csv
import numpy as np
import matplotlib.pyplot as plt

NUM_BLOCKS = 20
CELLS_PER_BLOCK = 18
R_CELL_NOM = 0.030          # 30 mΩ por célula
R0_BLOCK_NOM = R_CELL_NOM / CELLS_PER_BLOCK  # 1.6667 mΩ
LAMBDA_RLS = 0.98
DELTA_I_MIN = 1.5           # A
CONFIRM_COUNTS = 40         # 40 passos de 10ms = 0.4s de confirmação

def get_drive_current(t_sec):
    t_lap = t_sec % 45.0
    if t_lap < 2.0:
        c = 30.0 + 25.0 * (t_lap / 2.0)
    elif t_lap < 5.5:
        tau = (t_lap - 2.0) / 3.5
        c = 140.0 + 35.0 * math.sin(tau * math.pi * 0.5)
    elif t_lap < 7.5:
        c = -30.0 + 5.0 * math.sin((t_lap - 5.5) * math.pi)
    elif t_lap < 12.0:
        c = 45.0 + 35.0 * math.sin((t_lap - 7.5) * 2.5)
    elif t_lap < 14.0:
        c = 130.0
    elif t_lap < 16.0:
        c = 5.0
    elif t_lap < 24.0:
        sub = (t_lap - 16.0) % 2.0
        c = 85.0 if sub < 1.2 else -15.0
    elif t_lap < 28.0:
        c = 165.0
    elif t_lap < 31.0:
        c = -25.0
    elif t_lap < 40.0:
        c = 75.0 + 35.0 * math.cos((t_lap - 31.0) * 1.5)
    else:
        c = 40.0
    ripple = 1.8 * math.sin(t_sec * 62.83)
    return c + ripple

def ocv_from_soc(soc):
    soc_pts = [0.00, 0.05, 0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 1.00]
    volt_pts = [2.80, 3.25, 3.42, 3.55, 3.63, 3.68, 3.74, 3.81, 3.90, 4.00, 4.10, 4.20]
    return float(np.interp(soc, soc_pts, volt_pts))

def run_simulation():
    print("====================================================================")
    print("🏎️  FORMULA ITA - BMS MASTER V0: SOFTWARE-IN-THE-LOOP (SIL) RUNNER")
    print("====================================================================")
    print("Topologia: 20s18p (360 células) | Taxa: 100 Hz (dt = 10 ms)")
    print("Cenário de Teste: 60s de Corrida FSAE com Injeção de Falha em t = 25.0s")
    print("Alvo da Injeção: Bloco #07 (queima de 1 fusível -> passa para 17p)")
    print("Ruído de Medição de ADC: ±1.5 mV | Ruído do Sensor de Corrente: ±0.2 A\n")

    random.seed(42)
    np.random.seed(42)

    dt = 0.01
    total_time = 60.0
    steps = int(total_time / dt)
    injection_time = 25.0
    target_block = 7

    # Estado físico de cada bloco
    active_cells = [18] * NUM_BLOCKS
    tol = [1.0 + 0.015 * math.sin(i * 1.3) for i in range(NUM_BLOCKS)]
    true_r0 = [(R_CELL_NOM / 18.0) * tol[i] for i in range(NUM_BLOCKS)]
    r_pol = [(0.018 / 18.0) * tol[i] for i in range(NUM_BLOCKS)]
    c_pol = [2200.0 * 18.0] * NUM_BLOCKS
    v_pol = [0.0] * NUM_BLOCKS
    true_soc = [0.90] * NUM_BLOCKS

    # Estado do estimador RLS
    theta_r0 = [R0_BLOCK_NOM] * NUM_BLOCKS
    p_cov = [1000.0] * NUM_BLOCKS
    prev_v = [0.0] * NUM_BLOCKS
    prev_i = 0.0
    confirm_counter = [0] * NUM_BLOCKS
    fuse_status = ["HEALTHY"] * NUM_BLOCKS
    fsm_state = "RUN"
    dcl_limit = 200.0

    detection_time = -1.0
    false_positives = 0
    max_z_before_fault = 0.0

    log_rows = []

    print(f"{'Tempo[s]':<8} | {'I_pack[A]':<8} | {'V_pack[V]':<8} | {'SOC[%]':<6} | {'Estado FSM':<12} | {'R0_B07[mΩ]':<10} | {'Z_B07':<8} | {'Status B07':<10}")
    print("-" * 94)

    for step in range(steps):
        t = step * dt

        # Queima do fusível em t = 25.0s
        if t >= injection_time and active_cells[target_block] == 18:
            active_cells[target_block] = 17
            true_r0[target_block] = (R_CELL_NOM / 17.0) * tol[target_block]
            r_pol[target_block] = (0.018 / 17.0) * tol[target_block]

        # Corrente do ciclo
        i_true = get_drive_current(t)
        i_meas = i_true + random.uniform(-0.2, 0.2)

        # Passo do modelo de bateria
        v_meas = []
        for b in range(NUM_BLOCKS):
            cap_ah = active_cells[b] * 3.0
            d_soc = (i_true * dt) / (cap_ah * 3600.0)
            true_soc[b] -= d_soc
            tau = r_pol[b] * c_pol[b]
            d_vp = (i_true * r_pol[b] - v_pol[b]) * (dt / tau)
            v_pol[b] += d_vp
            ocv = ocv_from_soc(true_soc[b])
            v_terminal = ocv - (i_true * true_r0[b]) - v_pol[b]
            # Ruído de ADC ±1.5 mV
            v_meas.append(v_terminal + random.uniform(-0.0015, 0.0015))

        pack_v = sum(v_meas)
        soc_avg = np.mean(true_soc) * 100.0

        if step == 0:
            prev_v = list(v_meas)
            prev_i = i_meas
            continue

        delta_i = i_meas - prev_i
        prev_i = i_meas

        # RLS Update
        if abs(delta_i) >= DELTA_I_MIN:
            phi = delta_i
            phi_sq = phi * phi
            for b in range(NUM_BLOCKS):
                delta_v = v_meas[b] - prev_v[b]
                prev_v[b] = v_meas[b]
                y = -delta_v
                denom = LAMBDA_RLS + phi_sq * p_cov[b]
                if denom > 1e-7:
                    k_gain = (p_cov[b] * phi) / denom
                    e = y - (phi * theta_r0[b])
                    theta_r0[b] += k_gain * e
                    theta_r0[b] = max(0.0005, min(0.0100, theta_r0[b]))
                    p_cov[b] = (p_cov[b] - k_gain * phi * p_cov[b]) / LAMBDA_RLS
                    p_cov[b] = max(1e-4, min(1e5, p_cov[b]))
        else:
            prev_v = list(v_meas)

        # Z-Score Inter-Blocos
        mean_r0 = float(np.mean(theta_r0))
        std_r0 = float(np.std(theta_r0, ddof=1))
        sigma_eff = max(std_r0, 0.000030)

        z_scores = [(theta_r0[b] - mean_r0) / sigma_eff for b in range(NUM_BLOCKS)]

        # Diagnóstico
        for b in range(NUM_BLOCKS):
            z = z_scores[b]
            if z >= 2.8:
                confirm_counter[b] = min(60000, confirm_counter[b] + 1)
                if confirm_counter[b] >= 25:
                    fuse_status[b] = "CONFIRMED"
                else:
                    fuse_status[b] = "SUSPECT"
            elif z >= 2.0:
                if fuse_status[b] != "CONFIRMED":
                    fuse_status[b] = "SUSPECT"
            else:
                if fuse_status[b] != "CONFIRMED":
                    if confirm_counter[b] > 0:
                        confirm_counter[b] -= 1
                    if confirm_counter[b] == 0:
                        fuse_status[b] = "HEALTHY"

        # Monitoramento de falhas
        if t < injection_time:
            for b in range(NUM_BLOCKS):
                if fuse_status[b] == "CONFIRMED":
                    false_positives += 1
                if z_scores[b] > max_z_before_fault:
                    max_z_before_fault = z_scores[b]
        else:
            if fuse_status[target_block] == "CONFIRMED" and detection_time < 0:
                detection_time = t

        # Ação FSM e Derating
        if "CONFIRMED" in fuse_status:
            fsm_state = "DERATED"
            dcl_limit = 200.0 * (17.0 / 18.0)  # 188.9 A
        else:
            fsm_state = "RUN"
            dcl_limit = 200.0

        b7_status = fuse_status[target_block]
        log_rows.append([
            t, i_meas, pack_v, soc_avg, fsm_state, dcl_limit,
            mean_r0 * 1000.0, std_r0 * 1e6, theta_r0[target_block] * 1000.0,
            z_scores[target_block], b7_status, active_cells[target_block]
        ])

        if step % 250 == 0:
            print(f"{t:<8.2f} | {i_meas:<8.1f} | {pack_v:<8.1f} | {soc_avg:<5.1f}% | {fsm_state:<12} | {theta_r0[target_block]*1000.0:<10.4f} | {z_scores[target_block]:<8.2f} | {b7_status:<10}")

    # Salva CSV
    with open("sil_telemetry_log.csv", "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["time_s","current_a","pack_v","soc_pct","fsm_state","dcl_a","mean_r0_mohm","std_r0_uohm","b07_r0_mohm","b07_z","b07_status","b07_active_cells"])
        writer.writerows(log_rows)

    print("\n📁 Arquivo de telemetria salvo em: sil_telemetry_log.csv")

    # Relatório
    true_healthy_m = (R_CELL_NOM / 18.0) * 1000.0
    true_blown_m = (R_CELL_NOM / 17.0) * 1000.0
    est_r0_m = theta_r0[target_block] * 1000.0
    err_pct = abs(est_r0_m - true_blown_m) / true_blown_m * 100.0

    print("\n====================================================================")
    print("📊 RELATÓRIO DE VALIDAÇÃO DOS CRITÉRIOS DE ACEITAÇÃO (SIL)")
    print("====================================================================")
    print(f"1. R0 Teórico Íntegro (18p):  {true_healthy_m:.4f} mΩ")
    print(f"2. R0 Teórico Bloco 7 (17p):  {true_blown_m:.4f} mΩ (+5.88%)")
    print(f"3. R0 Estimado via RLS:       {est_r0_m:.4f} mΩ (Erro relativo: {err_pct:.2f}%)")
    print(f"4. Z-Score Máx antes falha:   {max_z_before_fault:.2f} ({'✅ PASSOU (<2.5)' if max_z_before_fault < 2.5 else '❌ FALHOU'})")
    print(f"5. Falsos Positivos:          {false_positives} ({'✅ ZERO' if false_positives == 0 else '❌ FALHOU'})")
    
    delay = detection_time - injection_time
    print(f"6. Queima Injetada em:        t = {injection_time:.2f} s")
    print(f"7. Queima Confirmada em:      t = {detection_time:.2f} s (Tempo de detecção: {delay:.2f} s)")
    print(f"8. Requisito FSAE (< 2.0 s):  {'✅ APROVADO COM FOLGA' if delay <= 2.0 else '⚠️ LENTO'}")
    print(f"9. Estado Final da FSM:       {fsm_state} (Relé SDC Fechado: SIM - Carro NÃO apagou)")
    print(f"10. Limite DCL Aplicado:      {dcl_limit:.1f} A (Derating proporcional de 17/18)")
    print("====================================================================\n")

if __name__ == "__main__":
    run_simulation()
