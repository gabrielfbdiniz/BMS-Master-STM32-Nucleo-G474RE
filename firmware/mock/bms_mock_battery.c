/**
 * @file bms_mock_battery.c
 * @brief Implementação do modelo físico Thévenin de 20 blocos com injeção de falhas
 */

#include "bms_mock_battery.h"
#include <string.h>
#include <math.h>

/* Tabela de OCV vs SOC (Li-Ion 3.6V nominal) */
static const float OCV_TABLE_SOC[] = {
    0.00f, 0.05f, 0.10f, 0.20f, 0.30f, 0.40f, 0.50f, 0.60f, 0.70f, 0.80f, 0.90f, 1.00f
};
static const float OCV_TABLE_VOLT[] = {
    2.800f, 3.250f, 3.420f, 3.550f, 3.630f, 3.680f, 3.740f, 3.810f, 3.900f, 4.000f, 4.100f, 4.200f
};
#define OCV_TABLE_SIZE (sizeof(OCV_TABLE_SOC) / sizeof(OCV_TABLE_SOC[0]))

/* Estado interno de cada bloco simulado */
typedef struct {
    float true_soc;              /**< SOC real (0.0 a 1.0) */
    float v_polarization;        /**< Tensão do ramo RC de polarização [V] */
    float true_r0;               /**< Resistência ôhmica real do bloco [Ω] */
    float r_polarization;        /**< Resistência de polarização [Ω] */
    float c_polarization;        /**< Capacitância de polarização [F] */
    float capacity_ah;           /**< Capacidade do bloco [Ah] */
    float temperature_c;         /**< Temperatura do bloco [°C] */
    uint8_t active_cells;        /**< 18 = íntegro, 17 = 1 fusível rompido */
} MockInternalBlock_t;

/* Estrutura de injeção de falha agendada */
typedef struct {
    bool scheduled;
    bool triggered;
    uint8_t target_block;
    float trigger_time_sec;
} ScheduledFault_t;

static MockInternalBlock_t s_blocks[BMS_NUM_BLOCKS_SERIES];
static MockBatteryConfig_t s_config;
static ScheduledFault_t    s_scheduled_fault;
static float               s_sim_time_sec = 0.0f;
static uint32_t            s_rng_state = 123456789U;

/* Gerador de ruído pseudo-aleatório uniforme entre -1.0 e +1.0 */
static float mock_rand_uniform(void)
{
    s_rng_state = (1103515245U * s_rng_state + 12345U) & 0x7FFFFFFFU;
    return ((float)s_rng_state / 1073741823.5f) - 1.0f;
}

/* Interpolação linear da OCV */
static float mock_calculate_ocv(float soc)
{
    if (soc <= OCV_TABLE_SOC[0]) {
        return OCV_TABLE_VOLT[0];
    }
    if (soc >= OCV_TABLE_SOC[OCV_TABLE_SIZE - 1]) {
        return OCV_TABLE_VOLT[OCV_TABLE_SIZE - 1];
    }

    for (size_t i = 0; i < OCV_TABLE_SIZE - 1; i++) {
        if (soc >= OCV_TABLE_SOC[i] && soc <= OCV_TABLE_SOC[i + 1]) {
            float frac = (soc - OCV_TABLE_SOC[i]) / (OCV_TABLE_SOC[i + 1] - OCV_TABLE_SOC[i]);
            return OCV_TABLE_VOLT[i] + frac * (OCV_TABLE_VOLT[i + 1] - OCV_TABLE_VOLT[i]);
        }
    }
    return 3.700f;
}

void bms_mock_battery_init(const MockBatteryConfig_t *config)
{
    if (config != NULL) {
        s_config = *config;
        s_rng_state = config->random_seed ? config->random_seed : 123456789U;
    } else {
        s_config.initial_soc = 0.90f; /* Começa com 90% de bateria */
        s_config.ambient_temp_c = 25.0f;
        s_config.enable_adc_noise = true;
        s_rng_state = 123456789U;
    }

    s_sim_time_sec = 0.0f;
    s_scheduled_fault.scheduled = false;
    s_scheduled_fault.triggered = false;

    /* Inicializa os 20 blocos com tolerância de fabricação natural de ±1.5% */
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        s_blocks[i].true_soc = s_config.initial_soc;
        s_blocks[i].v_polarization = 0.0f;
        s_blocks[i].active_cells = BMS_CELLS_PER_BLOCK_PARALLEL; /* 18 células */
        
        /* Inicializa os 20 blocos com modelo Samsung INR21700 (tolerância de fabricação de ±1.5%) */
        float tol = 1.0f + 0.015f * sinf((float)i * 1.3f);
        s_blocks[i].true_r0 = BMS_BLOCK_NOM_R0_OHM * tol;
        s_blocks[i].r_polarization = (0.010f / 18.0f) * tol;
        s_blocks[i].c_polarization = 2500.0f * 18.0f;
        s_blocks[i].capacity_ah = BMS_BLOCK_NOM_CAPACITY_AH;
        s_blocks[i].temperature_c = s_config.ambient_temp_c + 0.5f * sinf((float)i);
    }
}

void bms_mock_battery_schedule_fuse_blow(uint8_t block_idx, float event_time_sec)
{
    if (block_idx < BMS_NUM_BLOCKS_SERIES) {
        s_scheduled_fault.scheduled = true;
        s_scheduled_fault.triggered = false;
        s_scheduled_fault.target_block = block_idx;
        s_scheduled_fault.trigger_time_sec = event_time_sec;
    }
}

void bms_mock_battery_step(float dt_sec, float current_a, BmsPackTelemetry_t *out_telemetry)
{
    s_sim_time_sec += dt_sec;

    /* Verifica disparo da queima programada de fusível */
    if (s_scheduled_fault.scheduled && !s_scheduled_fault.triggered) {
        if (s_sim_time_sec >= s_scheduled_fault.trigger_time_sec) {
            uint8_t b = s_scheduled_fault.target_block;
            s_blocks[b].active_cells = 17; /* Uma célula queimou o fusível */
            
            /* Nova resistência equivalente de 17 células Samsung 21700 em paralelo (+5.88%) */
            float tol = 1.0f + 0.015f * sinf((float)b * 1.3f);
            s_blocks[b].true_r0 = (BMS_SINGLE_CELL_R0_OHM / 17.0f) * tol;
            s_blocks[b].r_polarization = (0.010f / 17.0f) * tol;
            s_blocks[b].capacity_ah = BMS_SINGLE_CELL_CAPACITY_AH * 17.0f;
            s_scheduled_fault.triggered = true;
        }
    }

    float pack_voltage_sum = 0.0f;
    float min_v = 1000.0f;
    float max_v = 0.0f;
    float max_temp = -100.0f;
    uint8_t min_v_idx = 0;
    uint8_t max_v_idx = 0;
    uint8_t max_temp_idx = 0;

    /* Ruído de medição de corrente (ex: sensor Hall LEM DHAB com ±0.2A de ruído) */
    float measured_current = current_a;
    if (s_config.enable_adc_noise) {
        measured_current += 0.2f * mock_rand_uniform();
    }

    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        MockInternalBlock_t *b = &s_blocks[i];

        /* Integração de Coulomb Counting do bloco: I > 0 é descarga */
        float d_soc = (current_a * dt_sec) / (b->capacity_ah * 3600.0f);
        b->true_soc -= d_soc;
        if (b->true_soc < 0.0f) b->true_soc = 0.0f;
        if (b->true_soc > 1.0f) b->true_soc = 1.0f;

        /* Dinâmica do ramo RC de polarização */
        float tau = b->r_polarization * b->c_polarization;
        if (tau > 0.001f) {
            float d_vp = (current_a * b->r_polarization - b->v_polarization) * (dt_sec / tau);
            b->v_polarization += d_vp;
        }

        /* OCV teórica em função do SOC */
        float ocv = mock_calculate_ocv(b->true_soc);

        /* Tensão terminal Thévenin física: V = OCV - I*R0 - Vp */
        float v_terminal_true = ocv - (current_a * b->true_r0) - b->v_polarization;

        /* Ruído de ADC simulado: ±1.5 mV (incompatibilidade, chaveamento e ruído térmico) */
        float v_measured = v_terminal_true;
        if (s_config.enable_adc_noise) {
            v_measured += 0.0015f * mock_rand_uniform();
        }

        /* Modelo térmico simples: Joules gerados por bloco */
        float p_loss = current_a * current_a * b->true_r0;
        /* Elevação térmica lenta + troca com ambiente */
        float d_temp = (p_loss * 0.00018f - 0.02f * (b->temperature_c - s_config.ambient_temp_c)) * dt_sec;
        b->temperature_c += d_temp;

        /* Grava dados de saída */
        if (out_telemetry != NULL) {
            out_telemetry->blocks[i].voltage_v = v_measured;
            out_telemetry->blocks[i].temperature_c = b->temperature_c;

            if (v_measured < min_v) {
                min_v = v_measured;
                min_v_idx = i;
            }
            if (v_measured > max_v) {
                max_v = v_measured;
                max_v_idx = i;
            }
            if (b->temperature_c > max_temp) {
                max_temp = b->temperature_c;
                max_temp_idx = i;
            }
        }

        pack_voltage_sum += v_measured;
    }

    if (out_telemetry != NULL) {
        out_telemetry->pack_voltage_v = pack_voltage_sum;
        out_telemetry->pack_current_a = measured_current;
        out_telemetry->v_cell_min_v = min_v;
        out_telemetry->v_cell_max_v = max_v;
        out_telemetry->min_v_block_idx = min_v_idx;
        out_telemetry->max_v_block_idx = max_v_idx;
        out_telemetry->temp_cell_max_c = max_temp;
        out_telemetry->max_temp_block_idx = max_temp_idx;
    }
}

float bms_mock_battery_get_true_r0(uint8_t block_idx)
{
    if (block_idx < BMS_NUM_BLOCKS_SERIES) {
        return s_blocks[block_idx].true_r0;
    }
    return 0.0f;
}

uint8_t bms_mock_battery_get_true_active_cells(uint8_t block_idx)
{
    if (block_idx < BMS_NUM_BLOCKS_SERIES) {
        return s_blocks[block_idx].active_cells;
    }
    return 0;
}
