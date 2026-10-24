/**
 * @file alg_soc.c
 * @brief Implementação de Coulomb Counting, fusão com OCV e cálculo de DCL/CCL
 */

#include "alg_soc.h"
#include <stddef.h>
#include <math.h>

static const float OCV_CURVE_SOC[] = {
    0.00f, 0.05f, 0.10f, 0.20f, 0.30f, 0.40f, 0.50f, 0.60f, 0.70f, 0.80f, 0.90f, 1.00f
};
static const float OCV_CURVE_VOLT[] = {
    2.800f, 3.250f, 3.420f, 3.550f, 3.630f, 3.680f, 3.740f, 3.810f, 3.900f, 4.000f, 4.100f, 4.200f
};
#define OCV_CURVE_POINTS (sizeof(OCV_CURVE_SOC) / sizeof(OCV_CURVE_SOC[0]))

static float s_estimated_soc = 0.90f;
static float s_rest_timer_sec = 0.0f;

/* Estimativa de SOC inversa a partir da OCV média */
static float soc_from_ocv(float ocv_volt)
{
    if (ocv_volt <= OCV_CURVE_VOLT[0]) return 0.0f;
    if (ocv_volt >= OCV_CURVE_VOLT[OCV_CURVE_POINTS - 1]) return 1.0f;

    for (size_t i = 0; i < OCV_CURVE_POINTS - 1; i++) {
        if (ocv_volt >= OCV_CURVE_VOLT[i] && ocv_volt <= OCV_CURVE_VOLT[i + 1]) {
            float frac = (ocv_volt - OCV_CURVE_VOLT[i]) / (OCV_CURVE_VOLT[i + 1] - OCV_CURVE_VOLT[i]);
            return OCV_CURVE_SOC[i] + frac * (OCV_CURVE_SOC[i + 1] - OCV_CURVE_SOC[i]);
        }
    }
    return 0.5f;
}

void alg_soc_init(BmsPackTelemetry_t *telemetry)
{
    if (telemetry != NULL) {
        float avg_v = telemetry->pack_voltage_v / (float)BMS_NUM_BLOCKS_SERIES;
        s_estimated_soc = soc_from_ocv(avg_v);
        telemetry->soc_percent = s_estimated_soc * 100.0f;
        telemetry->max_discharge_limit_a = BMS_MAX_DISCHARGE_CURRENT_A;
        telemetry->max_charge_limit_a = BMS_MAX_CHARGE_CURRENT_A;
    } else {
        s_estimated_soc = 0.90f;
    }
    s_rest_timer_sec = 0.0f;
}

void alg_soc_update(BmsPackTelemetry_t *telemetry, float dt_sec)
{
    if (telemetry == NULL) return;

    float current = telemetry->pack_current_a;

    /* 1. Integração de Coulomb Counting: SOC = SOC - (I * dt) / (Q_nom * 3600) */
    float delta_soc = (current * dt_sec) / (BMS_BLOCK_NOM_CAPACITY_AH * 3600.0f);
    s_estimated_soc -= delta_soc;

    /* 2. Filtro de repouso para recalibração por OCV */
    if (fabsf(current) < 0.5f) {
        s_rest_timer_sec += dt_sec;
        if (s_rest_timer_sec > 5.0f) { /* Em repouso por mais de 5s */
            float avg_v = telemetry->pack_voltage_v / (float)BMS_NUM_BLOCKS_SERIES;
            float ocv_soc = soc_from_ocv(avg_v);
            /* Fusão suave de primeira ordem */
            s_estimated_soc = 0.995f * s_estimated_soc + 0.005f * ocv_soc;
        }
    } else {
        s_rest_timer_sec = 0.0f;
    }

    if (s_estimated_soc < 0.0f) s_estimated_soc = 0.0f;
    if (s_estimated_soc > 1.0f) s_estimated_soc = 1.0f;

    telemetry->soc_percent = s_estimated_soc * 100.0f;

    /* 3. Cálculo de Limites Dinâmicos de Corrente (DCL / CCL) */
    float dcl = BMS_MAX_DISCHARGE_CURRENT_A;
    float ccl = BMS_MAX_CHARGE_CURRENT_A;

    /* Derating térmico se alguma célula ultrapassar 55 °C */
    if (telemetry->temp_cell_max_c > BMS_CELL_TEMP_WARN_C) {
        float temp_excess = telemetry->temp_cell_max_c - BMS_CELL_TEMP_WARN_C;
        float derate_factor = 1.0f - (temp_excess / (BMS_CELL_TEMP_MAX_C - BMS_CELL_TEMP_WARN_C));
        if (derate_factor < 0.1f) derate_factor = 0.1f;
        dcl *= derate_factor;
        ccl *= derate_factor;
    }

    /* Derating por tensão mínima se aproximando de 3.0V */
    if (telemetry->v_cell_min_v < BMS_CELL_V_WARN_LOW_MV / 1000.0f) {
        float v_margin = (telemetry->v_cell_min_v - (BMS_CELL_V_MIN_MV / 1000.0f)) / 0.2f;
        if (v_margin < 0.0f) v_margin = 0.0f;
        if (v_margin > 1.0f) v_margin = 1.0f;
        dcl *= v_margin;
    }

    /* Derating preventivo de Fusível Rompido: */
    /* Se algum bloco tem status CONFIRMED (17p), reduz o pico proporcionalmente para proteger as 17 células restantes */
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        if (telemetry->blocks[i].fuse_status == FUSE_STATUS_BLOWN_CONFIRMED) {
            float fuse_derate = 17.0f / 18.0f; /* -5.56% */
            if (dcl > (BMS_MAX_DISCHARGE_CURRENT_A * fuse_derate)) {
                dcl = BMS_MAX_DISCHARGE_CURRENT_A * fuse_derate;
            }
            break;
        }
    }

    telemetry->max_discharge_limit_a = dcl;
    telemetry->max_charge_limit_a = ccl;
}
