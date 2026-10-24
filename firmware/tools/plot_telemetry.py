"""
@file plot_telemetry.py
@brief Gerador de gráficos e análise estatística dos dados SIL do BMS Master V0
"""

import os
import sys
import pandas as pd
import matplotlib.pyplot as plt

def main():
    csv_file = "sil_telemetry_log.csv"
    if not os.path.exists(csv_file):
        # Tenta no diretório tests_sil
        alt_path = os.path.join(os.path.dirname(__file__), "..", "tests_sil", "sil_telemetry_log.csv")
        if os.path.exists(alt_path):
            csv_file = alt_path
        else:
            print(f"Erro: Arquivo '{csv_file}' não encontrado. Execute a simulação SIL primeiro.")
            sys.exit(1)

    df = pd.read_csv(csv_file)
    print(f"Lendo telemetria: {len(df)} amostras (dt = 10 ms)")

    fig, axs = plt.subplots(4, 1, figsize=(12, 10), sharex=True)
    fig.suptitle("Formula ITA — BMS Master V0: Validação SIL (Método 1 - Queima de Fusível)", fontsize=14, fontweight='bold')

    # Subplot 1: Corrente e Tensão
    color = 'tab:blue'
    axs[0].set_ylabel("Corrente Pack [A]", color=color, fontweight='bold')
    axs[0].plot(df['time_s'], df['current_a'], color=color, label="Corrente (Descarga > 0 / Regen < 0)", alpha=0.85)
    axs[0].tick_params(axis='y', labelcolor=color)
    axs[0].grid(True, linestyle='--', alpha=0.5)

    ax0_twin = axs[0].twinx()
    color = 'tab:red'
    ax0_twin.set_ylabel("Tensão Pack [V]", color=color, fontweight='bold')
    ax0_twin.plot(df['time_s'], df['pack_v'], color=color, linestyle='-', label="Tensão Total (20S)", alpha=0.7)
    ax0_twin.tick_params(axis='y', labelcolor=color)

    # Subplot 2: R0 do Bloco 7 vs Média do Pack
    axs[1].plot(df['time_s'], df['b07_r0_mohm'], label="R0 Estimado Bloco #07 [mΩ]", color='crimson', lw=2)
    axs[1].plot(df['time_s'], df['mean_r0_mohm'], label="Média Inter-Blocos (µ_R) [mΩ]", color='black', linestyle='--', lw=1.5)
    axs[1].axvline(x=25.0, color='orange', linestyle=':', lw=2, label="Injeção da Queima (t = 25.0s)")
    axs[1].set_ylabel("Resistência R0 [mΩ]", fontweight='bold')
    axs[1].legend(loc="upper left")
    axs[1].grid(True, linestyle='--', alpha=0.5)

    # Subplot 3: Z-Score do Bloco 7
    axs[2].plot(df['time_s'], df['b07_z'], label="Z-Score Bloco #07", color='purple', lw=2)
    axs[2].axhline(y=2.2, color='gold', linestyle='--', label="Limiar de Suspeita (Z = 2.2)")
    axs[2].axhline(y=3.0, color='red', linestyle='--', label="Limiar de Confirmação (Z = 3.0)")
    axs[2].axvline(x=25.0, color='orange', linestyle=':', lw=2, label="Injeção da Queima (t = 25.0s)")
    axs[2].set_ylabel("Z-Score", fontweight='bold')
    axs[2].legend(loc="upper left")
    axs[2].grid(True, linestyle='--', alpha=0.5)

    # Subplot 4: Limite Dinâmico de Corrente (DCL) e Ação de Derating
    axs[3].plot(df['time_s'], df['dcl_a'], label="DCL (Discharge Current Limit) [A]", color='green', lw=2)
    axs[3].axvline(x=25.0, color='orange', linestyle=':', lw=2)
    axs[3].set_ylabel("Limite DCL [A]", fontweight='bold')
    axs[3].set_xlabel("Tempo de Simulação [s]", fontweight='bold')
    axs[3].legend(loc="lower left")
    axs[3].grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    output_png = "sil_telemetry_plot.png"
    plt.savefig(output_png, dpi=200)
    print(f"Gráfico gerado com sucesso: {output_png}")

if __name__ == "__main__":
    main()
