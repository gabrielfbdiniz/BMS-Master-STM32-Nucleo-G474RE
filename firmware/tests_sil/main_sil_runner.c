/**
 * @file main_sil_runner.c
 * @brief Runner de Simulação Software-in-the-Loop (SIL) no PC
 * @details Simula 60 segundos de corrida FSAE com injeção de rompimento de fusível em t=25.0s
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "bms_types.h"
#include "bms_mock_battery.h"
#include "drive_cycle.h"
#include "alg_fuse_detect.h"
#include "alg_soc.h"
#include "fsm_master.h"

int main(void)
{
    printf("====================================================================\n");
    printf("🏎️  FORMULA ITA - BMS MASTER V0: SOFTWARE-IN-THE-LOOP (SIL) RUNNER\n");
    printf("====================================================================\n");
    printf("Células: Samsung INR21700-40T (20s18p = 360 células) | Taxa: 100 Hz\n");
    printf("Condição de Corrida: SEM FREIO REGENERATIVO (Frenagem mecânica pura)\n");
    printf("Cenário de Teste: 60s de Corrida FSAE com Injeção de Falha em t = 25.0s\n");
    printf("Alvo da Injeção: Bloco #07 (queima de 1 fusível -> passa de 18p para 17p)\n");
    printf("Ruído de Medição de ADC: ±1.5 mV | Ruído de Sensor de Corrente: ±0.2 A\n\n");

    /* 1. Inicializações */
    MockBatteryConfig_t mock_cfg = {
        .initial_soc = 0.90f,
        .ambient_temp_c = 25.0f,
        .enable_adc_noise = true,
        .random_seed = 42U
    };
    bms_mock_battery_init(&mock_cfg);
    drive_cycle_init();

    BmsPackTelemetry_t telemetry;
    bms_mock_battery_step(0.01f, 0.0f, &telemetry);

    alg_soc_init(&telemetry);
    alg_fuse_detect_init(NULL);
    fsm_master_init(&telemetry);

    /* 2. AUTO-IDENTIFICAÇÃO PRÉ-PARTIDA (PRE-FLIGHT HEALTH CHECK) */
    printf(">> Executando Pre-Flight Impedance Check (Auto-Teste Pré-Partida)...\n");
    float v_baseline[BMS_NUM_BLOCKS_SERIES];
    float v_pulse[BMS_NUM_BLOCKS_SERIES];
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        v_baseline[i] = telemetry.blocks[i].voltage_v;
    }
    /* Simula um pulso diagnóstico de 20A por 100 ms antes da largada */
    bms_mock_battery_step(0.10f, 20.0f, &telemetry);
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        v_pulse[i] = telemetry.blocks[i].voltage_v;
    }

    PreflightReport_t preflight_rep;
    bool preflight_ok = alg_fuse_detect_run_preflight(v_baseline, v_pulse, 20.0f, &preflight_rep);
    printf("   Status Pré-Partida: %s (Maior Z inicial: %.2f no Bloco #%02d)\n",
           preflight_ok ? "✅ APROVADO (Todos os 20 blocos íntegros)" : "⚠️ ANOMALIA DETECTADA",
           preflight_rep.max_initial_z_score, preflight_rep.suspect_block_idx);
    printf("   R0 Nominal Médio Identificado: %.4f mΩ\n\n",
           preflight_rep.identified_r0_mohm[0]);

    /* Solicita fechamento de contatores para corrida */
    fsm_master_request_run();

    /* Agenda rompimento de fusível no Bloco 7 aos 25.0s */
    const float INJECTION_TIME_SEC = 25.0f;
    const uint8_t TARGET_BLOCK = 7;
    bms_mock_battery_schedule_fuse_blow(TARGET_BLOCK, INJECTION_TIME_SEC);

    /* Abre arquivo para log CSV */
    FILE *f_csv = fopen("sil_telemetry_log.csv", "w");
    if (f_csv != NULL) {
        fprintf(f_csv, "time_s,current_a,pack_v,soc_pct,fsm_state,dcl_a,mean_r0_mohm,std_r0_uohm,b07_r0_mohm,b07_z,b07_status,b07_active_cells\n");
    }

    /* Variáveis de diagnóstico do teste */
    const float DT_SEC = 0.01f;
    const float TOTAL_TIME_SEC = 60.0f;
    const size_t TOTAL_STEPS = (size_t)(TOTAL_TIME_SEC / DT_SEC);

    float detection_timestamp_sec = -1.0f;
    bool false_positive_detected = false;
    float max_z_before_fault = 0.0f;

    printf("Iniciando loop de simulação temporal...\n");
    printf("%-8s | %-8s | %-8s | %-6s | %-12s | %-10s | %-8s | %-10s\n",
           "Tempo[s]", "I_pack[A]", "V_pack[V]", "SOC[%]", "Estado FSM", "R0_B07[mΩ]", "Z_B07", "Status B07");
    printf("----------------------------------------------------------------------------------------------\n");

    for (size_t step = 0; step < TOTAL_STEPS; step++) {
        float sim_time = (float)step * DT_SEC;

        /* 1. Obter corrente do perfil de corrida */
        float current_a = drive_cycle_get_current(sim_time);

        /* 2. Executar modelo de simulação da bateria */
        bms_mock_battery_step(DT_SEC, current_a, &telemetry);

        /* 3. Executar algoritmos do BMS Master */
        fsm_master_update(&telemetry, DT_SEC);
        alg_soc_update(&telemetry, DT_SEC);
        alg_fuse_detect_update(&telemetry, DT_SEC);

        /* Estatísticas globais */
        float mean_r0 = 0.0f, std_r0 = 0.0f;
        alg_fuse_detect_get_stats(&mean_r0, &std_r0);

        CellBlockData_t *b7 = &telemetry.blocks[TARGET_BLOCK];

        /* Monitoramento de Falsos Positivos antes de t = 25.0s (ignora os primeiros 500 ms de estabilização) */
        if (sim_time > 0.5f && sim_time < INJECTION_TIME_SEC) {
            for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
                if (telemetry.blocks[i].fuse_status == FUSE_STATUS_BLOWN_CONFIRMED) {
                    false_positive_detected = true;
                }
                if (telemetry.blocks[i].z_score > max_z_before_fault) {
                    max_z_before_fault = telemetry.blocks[i].z_score;
                }
            }
        } else if (sim_time >= INJECTION_TIME_SEC) {
            /* Monitoramento pós-falha: registra primeiro instante de confirmação */
            if (b7->fuse_status == FUSE_STATUS_BLOWN_CONFIRMED && detection_timestamp_sec < 0.0f) {
                detection_timestamp_sec = sim_time;
            }
        }

        /* Gravação no CSV */
        if (f_csv != NULL) {
            fprintf(f_csv, "%.3f,%.2f,%.2f,%.2f,%d,%.1f,%.4f,%.2f,%.4f,%.2f,%d,%d\n",
                    sim_time,
                    telemetry.pack_current_a,
                    telemetry.pack_voltage_v,
                    telemetry.soc_percent,
                    telemetry.state,
                    telemetry.max_discharge_limit_a,
                    mean_r0 * 1000.0f,
                    std_r0 * 1e6f,
                    b7->r0_estimated_ohm * 1000.0f,
                    b7->z_score,
                    b7->fuse_status,
                    b7->active_cells);
        }

        /* Impressão periódica a cada 2.5 segundos no console */
        if (step % 250 == 0) {
            const char *state_str = "RUN";
            if (telemetry.state == BMS_STATE_DERATED) state_str = "DERATED";
            else if (telemetry.state == BMS_STATE_FAULT) state_str = "FAULT";
            else if (telemetry.state == BMS_STATE_PRECHARGE) state_str = "PRECHARGE";

            const char *status_str = "HEALTHY";
            if (b7->fuse_status == FUSE_STATUS_SUSPECT) status_str = "SUSPECT";
            else if (b7->fuse_status == FUSE_STATUS_BLOWN_CONFIRMED) status_str = "CONFIRMED";

            printf("%-8.2f | %-8.1f | %-8.1f | %-5.1f%% | %-12s | %-10.4f | %-8.2f | %-10s\n",
                   sim_time,
                   telemetry.pack_current_a,
                   telemetry.pack_voltage_v,
                   telemetry.soc_percent,
                   state_str,
                   b7->r0_estimated_ohm * 1000.0f,
                   b7->z_score,
                   status_str);
        }
    }

    if (f_csv != NULL) {
        fclose(f_csv);
        printf("\n📁 Arquivo de telemetria salvo com sucesso em: sil_telemetry_log.csv\n");
    }

    /* 4. Relatório de Validação e Critérios de Aceitação */
    printf("\n====================================================================\n");
    printf("📊 RELATÓRIO DE VALIDAÇÃO DOS CRITÉRIOS DE ACEITAÇÃO (SIL)\n");
    printf("====================================================================\n");

    float true_r0_healthy = BMS_SINGLE_CELL_R0_OHM / 18.0f * 1000.0f;
    float true_r0_blown   = BMS_SINGLE_CELL_R0_OHM / 17.0f * 1000.0f;
    float estimated_r0_b7 = telemetry.blocks[TARGET_BLOCK].r0_estimated_ohm * 1000.0f;
    float error_pct = fabsf(estimated_r0_b7 - true_r0_blown) / true_r0_blown * 100.0f;

    printf("1. Célula Utilizada:          Samsung INR21700 (Rcell = %.1f mΩ nominal)\n", BMS_SINGLE_CELL_R0_OHM * 1000.0f);
    printf("2. Freio Regenerativo:        DESATIVADO (Frenagem Mecânica Pura)\n");
    printf("3. R0 Teórico Íntegro (18p):  %.4f mΩ\n", true_r0_healthy);
    printf("4. R0 Teórico Bloco 7 (17p):  %.4f mΩ (+5.88%%)\n", true_r0_blown);
    printf("5. R0 Estimado via RLS:       %.4f mΩ (Erro relativo: %.2f%%)\n", estimated_r0_b7, error_pct);
    printf("6. Z-Score Máximo pré-falha:  %.2f (Margem de segurança > 2.0: %s)\n",
           max_z_before_fault, (max_z_before_fault < 2.0f) ? "✅ PASSOU" : "⚠️ ATENÇÃO");
    printf("7. Falsos Positivos:          %s\n", false_positive_detected ? "❌ SIM (REPROVADO)" : "✅ ZERO (PASSOU)");

    if (detection_timestamp_sec >= 0.0f) {
        float delay = detection_timestamp_sec - INJECTION_TIME_SEC;
        printf("6. Queima Injetada em:        t = %.2f s\n", INJECTION_TIME_SEC);
        printf("7. Queima Confirmada em:      t = %.2f s (Tempo de detecção: %.2f s)\n", detection_timestamp_sec, delay);
        printf("8. Requisito FSAE (< 2.0 s):  %s\n", (delay <= 2.0f) ? "✅ APROVADO COM FOLGA" : "⚠️ LENTO");
    } else {
        printf("6. Falha NÃO foi confirmada pelo algoritmo! ❌ REPROVADO\n");
    }

    printf("9. Estado Final da FSM:       %s (Relé SDC Fechado: %s)\n",
           (telemetry.state == BMS_STATE_DERATED) ? "✅ BMS_STATE_DERATED" : "❌ OUTRO",
           telemetry.sdc_relay_closed ? "SIM (Carro não apagou na pista)" : "NÃO");
    printf("10. Limite DCL Aplicado:      %.1f A (Derating proporcional de 17/18)\n", telemetry.max_discharge_limit_a);

    printf("====================================================================\n");

    return (false_positive_detected || detection_timestamp_sec < 0.0f) ? 1 : 0;
}
