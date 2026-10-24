/**
 * @file fsm_master.c
 * @brief Implementação da FSM e checagem de conformidade de segurança FSAE
 */

#include "fsm_master.h"
#include <stddef.h>
#include <stdbool.h>

#define PRECHARGE_TIMEOUT_SEC  (0.800f) /**< Timeout máximo de pré-carga: 800 ms */
#define PRECHARGE_THRESHOLD    (0.950f) /**< 95% de Vpack */

static bool  s_run_requested = false;
static float s_precharge_timer = 0.0f;

void fsm_master_init(BmsPackTelemetry_t *telemetry)
{
    s_run_requested = false;
    s_precharge_timer = 0.0f;

    if (telemetry != NULL) {
        telemetry->state = BMS_STATE_INIT;
        telemetry->faults.raw_flags = 0;
        telemetry->sdc_relay_closed = true; /* Inicialmente seguro */
        telemetry->dclink_voltage_v = 0.0f;
    }
}

void fsm_master_request_run(void)
{
    s_run_requested = true;
}

void fsm_master_request_standby(void)
{
    s_run_requested = false;
}

void fsm_master_update(BmsPackTelemetry_t *telemetry, float dt_sec)
{
    if (telemetry == NULL) return;

    /* 1. Checagem prioritária de falhas críticas de segurança (FSAE EV4) */
    if (telemetry->v_cell_max_v > (BMS_CELL_V_MAX_MV / 1000.0f)) {
        telemetry->faults.bits.over_voltage = 1;
    }
    if (telemetry->v_cell_min_v < (BMS_CELL_V_MIN_MV / 1000.0f)) {
        telemetry->faults.bits.under_voltage = 1;
    }
    if (telemetry->temp_cell_max_c >= BMS_CELL_TEMP_MAX_C) {
        telemetry->faults.bits.over_temperature = 1;
    }

    /* Se houver qualquer falha crítica de segurança, transiciona imediatamente para FAULT */
    if (telemetry->faults.raw_flags != 0) {
        telemetry->state = BMS_STATE_FAULT;
        telemetry->sdc_relay_closed = false; /* Corta o relé SDC (AMS Trip físico) */
        telemetry->dclink_voltage_v = 0.0f;
        return;
    }

    /* 2. Execução da Máquina de Estados Operacional */
    switch (telemetry->state) {
        case BMS_STATE_INIT:
            /* Autoteste concluído com sucesso */
            telemetry->sdc_relay_closed = true;
            telemetry->state = BMS_STATE_STANDBY;
            break;

        case BMS_STATE_STANDBY:
            telemetry->dclink_voltage_v = 0.0f;
            if (s_run_requested) {
                s_precharge_timer = 0.0f;
                telemetry->state = BMS_STATE_PRECHARGE;
            }
            break;

        case BMS_STATE_PRECHARGE:
            s_precharge_timer += dt_sec;
            /* Simulação do carregamento do capacitor de DC Link via resistor de 47 Ohms (tau ~ 47 ms) */
            if (telemetry->dclink_voltage_v < telemetry->pack_voltage_v) {
                float v_diff = telemetry->pack_voltage_v - telemetry->dclink_voltage_v;
                telemetry->dclink_voltage_v += v_diff * (dt_sec / 0.047f);
            }

            /* Checa se atingiu 95% da tensão do acumulador */
            if (telemetry->dclink_voltage_v >= (telemetry->pack_voltage_v * PRECHARGE_THRESHOLD)) {
                telemetry->dclink_voltage_v = telemetry->pack_voltage_v;
                telemetry->state = BMS_STATE_RUN;
            } 
            else if (s_precharge_timer >= PRECHARGE_TIMEOUT_SEC) {
                /* Falha no circuito de pré-carga */
                telemetry->faults.bits.precharge_timeout = 1;
                telemetry->state = BMS_STATE_FAULT;
                telemetry->sdc_relay_closed = false;
            }
            break;

        case BMS_STATE_RUN:
            telemetry->dclink_voltage_v = telemetry->pack_voltage_v;

            /* Checa se algum fusível foi confirmado como rompido */
            for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
                if (telemetry->blocks[i].fuse_status == FUSE_STATUS_BLOWN_CONFIRMED) {
                    telemetry->state = BMS_STATE_DERATED;
                    break;
                }
            }

            if (!s_run_requested) {
                telemetry->state = BMS_STATE_STANDBY;
            }
            break;

        case BMS_STATE_DERATED:
            telemetry->dclink_voltage_v = telemetry->pack_voltage_v;

            /* Permanece em derating até que o veículo seja desligado ou comando de Standby */
            if (!s_run_requested) {
                telemetry->state = BMS_STATE_STANDBY;
            }
            break;

        case BMS_STATE_CHARGING:
            /* Modo de carregador de bancada */
            break;

        case BMS_STATE_FAULT:
        default:
            /* Latch permanente: SDC aberto, requer power cycle */
            telemetry->sdc_relay_closed = false;
            telemetry->dclink_voltage_v = 0.0f;
            break;
    }
}
