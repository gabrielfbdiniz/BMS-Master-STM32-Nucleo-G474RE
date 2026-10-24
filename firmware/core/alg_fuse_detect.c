/**
 * @file alg_fuse_detect.c
 * @brief Implementação do detector online de queima de fusível via RLS + Z-Score
 */

#include "alg_fuse_detect.h"
#include <math.h>
#include <string.h>

#define P_MIN_VAL      (1e-4f)
#define P_MAX_VAL      (1e5f)
#define R0_MIN_OHM     (0.0002f) /**< 0.2 mΩ */
#define R0_MAX_OHM     (0.0050f) /**< 5.0 mΩ */
#define SIGMA_FLOOR    (0.000010f) /**< Piso mínimo de desvio padrão: 10 µΩ (Samsung INR21700) */

/* Estado interno do estimador RLS para cada um dos 20 blocos */
typedef struct {
    float theta_r0;              /**< Estimativa de R0 [Ω] */
    float p_cov;                 /**< Covariância escalar P */
    float prev_voltage_v;        /**< Tensão terminal no passo anterior [V] */
    float baseline_r0;           /**< Linha de base identificada no Pre-Flight [Ω] */
    uint16_t confirm_counter;    /**< Contador de persistência Z-score */
    FuseStatus_t status;         /**< Status de diagnóstico atual */
} BlockRlsState_t;

static BlockRlsState_t   s_rls[BMS_NUM_BLOCKS_SERIES];
static FuseDetectConfig_t s_cfg;
static float             s_prev_current_a = 0.0f;
static bool              s_is_first_sample = true;
static float             s_mean_r0 = BMS_BLOCK_NOM_R0_OHM;
static float             s_std_r0 = 0.000050f;

bool alg_fuse_detect_run_preflight(const float v_baseline[BMS_NUM_BLOCKS_SERIES],
                                   const float v_under_pulse[BMS_NUM_BLOCKS_SERIES],
                                   float pulse_current_a,
                                   PreflightReport_t *out_report)
{
    if (v_baseline == NULL || v_under_pulse == NULL || fabsf(pulse_current_a) < 1.0f) {
        return false;
    }

    float r0_identified[BMS_NUM_BLOCKS_SERIES];
    float sum_r0 = 0.0f;

    /* 1. Cálculo da resistência de cada bloco pelo pulso controlado */
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        float delta_v = v_baseline[i] - v_under_pulse[i];
        float r = delta_v / pulse_current_a;
        if (r < R0_MIN_OHM) r = R0_MIN_OHM;
        if (r > R0_MAX_OHM) r = R0_MAX_OHM;
        r0_identified[i] = r;
        sum_r0 += r;

        /* Pré-carrega o estimador RLS com a impedância identificada */
        s_rls[i].theta_r0 = r;
        s_rls[i].baseline_r0 = r;
        s_rls[i].p_cov = 50.0f; /* Covariância menor pois já temos medição direta */
    }

    /* 2. Estatística inter-blocos */
    s_mean_r0 = sum_r0 / (float)BMS_NUM_BLOCKS_SERIES;
    float sum_sq = 0.0f;
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        float d = r0_identified[i] - s_mean_r0;
        sum_sq += d * d;
    }
    s_std_r0 = sqrtf(sum_sq / (float)(BMS_NUM_BLOCKS_SERIES - 1));
    float sigma_eff = (s_std_r0 > SIGMA_FLOOR) ? s_std_r0 : SIGMA_FLOOR;

    /* 3. Avaliação de conformidade Pré-Voo */
    bool all_passed = true;
    uint8_t suspect_idx = 0;
    float max_z = -100.0f;

    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        float z = (r0_identified[i] - s_mean_r0) / sigma_eff;
        if (z > max_z) {
            max_z = z;
            suspect_idx = i;
        }

        if (out_report != NULL) {
            out_report->identified_r0_mohm[i] = r0_identified[i] * 1000.0f;
        }

        if (z >= s_cfg.z_blown_threshold) {
            all_passed = false;
            s_rls[i].status = FUSE_STATUS_BLOWN_CONFIRMED;
            s_rls[i].confirm_counter = s_cfg.confirm_counts;
        } else if (z >= s_cfg.z_suspect_threshold) {
            s_rls[i].status = FUSE_STATUS_SUSPECT;
        } else {
            s_rls[i].status = FUSE_STATUS_HEALTHY;
        }
    }

    if (out_report != NULL) {
        out_report->preflight_passed = all_passed;
        out_report->suspect_block_idx = suspect_idx;
        out_report->max_initial_z_score = max_z;
    }

    return all_passed;
}

void alg_fuse_detect_init(const FuseDetectConfig_t *config)
{
    if (config != NULL) {
        s_cfg = *config;
    } else {
        s_cfg.lambda = 0.98f;
        s_cfg.p_init = 1000.0f;
        s_cfg.delta_i_min_a = 1.5f;        /* Exige pelo menos 1.5 A de transitório para atualizar RLS */
        s_cfg.z_suspect_threshold = 2.2f;
        s_cfg.z_blown_threshold = 3.0f;
        s_cfg.confirm_counts = 25;         /* 25 passos = 0.25s a 100 Hz */
    }

    s_prev_current_a = 0.0f;
    s_is_first_sample = true;
    s_mean_r0 = BMS_BLOCK_NOM_R0_OHM;
    s_std_r0 = 0.000050f;

    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        s_rls[i].theta_r0 = BMS_BLOCK_NOM_R0_OHM; /* Inicia com valor nominal */
        s_rls[i].baseline_r0 = BMS_BLOCK_NOM_R0_OHM;
        s_rls[i].p_cov = s_cfg.p_init;
        s_rls[i].prev_voltage_v = 0.0f;
        s_rls[i].confirm_counter = 0;
        s_rls[i].status = FUSE_STATUS_HEALTHY;
    }
}

void alg_fuse_detect_update(BmsPackTelemetry_t *telemetry, float dt_sec)
{
    (void)dt_sec;

    if (telemetry == NULL) {
        return;
    }

    float current = telemetry->pack_current_a;

    if (s_is_first_sample) {
        s_prev_current_a = current;
        for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
            s_rls[i].prev_voltage_v = telemetry->blocks[i].voltage_v;
        }
        s_is_first_sample = false;
        return;
    }

    /* Variação incremental de corrente */
    float delta_i = current - s_prev_current_a;
    s_prev_current_a = current;

    /* CAMADA 1: Identificação Paramétrica On-line via RLS Escalar */
    /* Excitation Gate: Somente atualiza RLS se houver variação real de corrente */
    if (fabsf(delta_i) >= s_cfg.delta_i_min_a) {
        float phi = delta_i;
        float phi_sq = phi * phi;

        for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
            BlockRlsState_t *st = &s_rls[i];
            float v_now = telemetry->blocks[i].voltage_v;
            float delta_v = v_now - st->prev_voltage_v;
            st->prev_voltage_v = v_now;

            /* Modelo incremental: delta_v = - R0 * delta_i  ==>  y = -delta_v = phi * R0 */
            float y = -delta_v;

            /* Ganho de Kalman/RLS: K = (P * phi) / (lambda + phi^2 * P) */
            float denom = s_cfg.lambda + phi_sq * st->p_cov;
            if (denom > 1e-7f) {
                float k_gain = (st->p_cov * phi) / denom;
                
                /* Erro de inovação/predição */
                float e = y - (phi * st->theta_r0);

                /* Atualização do parâmetro */
                st->theta_r0 += k_gain * e;

                /* Limite físico de R0 para evitar estimativas aberrantes */
                if (st->theta_r0 < R0_MIN_OHM) st->theta_r0 = R0_MIN_OHM;
                if (st->theta_r0 > R0_MAX_OHM) st->theta_r0 = R0_MAX_OHM;

                /* Atualização da covariância: P = (1/lambda) * (P - K * phi * P) */
                float new_p = (st->p_cov - k_gain * phi * st->p_cov) / s_cfg.lambda;
                if (new_p < P_MIN_VAL) new_p = P_MIN_VAL;
                if (new_p > P_MAX_VAL) new_p = P_MAX_VAL;
                st->p_cov = new_p;
            }
        }
    } else {
        /* Se a corrente estiver em repouso/estacionária, apenas armazena as tensões anteriores */
        for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
            s_rls[i].prev_voltage_v = telemetry->blocks[i].voltage_v;
        }
    }

    /* CAMADA 2: Análise Estatística Inter-Blocos (Z-Score) */
    /* 1. Média aritmética de R0 */
    float sum_r0 = 0.0f;
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        sum_r0 += s_rls[i].theta_r0;
    }
    s_mean_r0 = sum_r0 / (float)BMS_NUM_BLOCKS_SERIES;

    /* 2. Desvio padrão amostral */
    float sum_sq_diff = 0.0f;
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        float diff = s_rls[i].theta_r0 - s_mean_r0;
        sum_sq_diff += diff * diff;
    }
    float variance = sum_sq_diff / (float)(BMS_NUM_BLOCKS_SERIES - 1);
    s_std_r0 = sqrtf(variance);

    /* Piso para evitar divisão por zero ou ruído residual microscópico */
    float sigma_eff = (s_std_r0 > SIGMA_FLOOR) ? s_std_r0 : SIGMA_FLOOR;

    /* CAMADA 3: Máquina de Estados de Diagnóstico por Bloco */
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        BlockRlsState_t *st = &s_rls[i];
        float z = (st->theta_r0 - s_mean_r0) / sigma_eff;
        float delta_pct = (st->baseline_r0 > 1e-5f) ? ((st->theta_r0 - st->baseline_r0) / st->baseline_r0) : 0.0f;

        telemetry->blocks[i].r0_estimated_ohm = st->theta_r0;
        telemetry->blocks[i].z_score = z;

        /* Critério Robusto: Z-Score >= 3.0 OU Z-Score >= 2.5 com desvio persistente da baseline */
        bool anomaly_detected = (z >= s_cfg.z_blown_threshold) || (z >= 2.5f && delta_pct >= 0.048f);

        if (anomaly_detected) {
            /* Desvio positivo confirmado de queima de fusível */
            if (st->confirm_counter < 60000U) {
                st->confirm_counter++;
            }
            if (st->confirm_counter >= s_cfg.confirm_counts) {
                st->status = FUSE_STATUS_BLOWN_CONFIRMED;
                telemetry->blocks[i].active_cells = 17;
            } else {
                st->status = FUSE_STATUS_SUSPECT;
            }
        } 
        else if (z >= s_cfg.z_suspect_threshold || (z >= 1.8f && delta_pct >= 0.035f)) {
            /* Desvio intermediário */
            if (st->status != FUSE_STATUS_BLOWN_CONFIRMED) {
                st->status = FUSE_STATUS_SUSPECT;
            }
        } 
        else {
            /* Faixa normal: se o alarme ainda não foi travado, decrementa o contador (anti-chattering) */
            if (st->status != FUSE_STATUS_BLOWN_CONFIRMED) {
                if (st->confirm_counter > 0) {
                    st->confirm_counter--;
                }
                if (st->confirm_counter == 0) {
                    st->status = FUSE_STATUS_HEALTHY;
                    telemetry->blocks[i].active_cells = BMS_CELLS_PER_BLOCK_PARALLEL;
                }
            }
        }

        telemetry->blocks[i].fuse_status = st->status;
        telemetry->blocks[i].suspect_counter = st->confirm_counter;
    }
}

bool alg_fuse_detect_has_blown_fuse(uint8_t *out_failed_block_idx)
{
    for (uint8_t i = 0; i < BMS_NUM_BLOCKS_SERIES; i++) {
        if (s_rls[i].status == FUSE_STATUS_BLOWN_CONFIRMED) {
            if (out_failed_block_idx != NULL) {
                *out_failed_block_idx = i;
            }
            return true;
        }
    }
    return false;
}

void alg_fuse_detect_get_stats(float *out_mean, float *out_std)
{
    if (out_mean != NULL) *out_mean = s_mean_r0;
    if (out_std != NULL)  *out_std = s_std_r0;
}
