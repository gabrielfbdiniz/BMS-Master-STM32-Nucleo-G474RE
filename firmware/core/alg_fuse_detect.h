/**
 * @file alg_fuse_detect.h
 * @brief Algoritmo Método 1: Detecção dinâmica de queima de fusível em topologia 18p
 * @details Utiliza RLS incremental (Delta V / Delta I) com lambda=0.98 + Z-score inter-blocos
 *          e rotina de Identificação Estática Pré-Partida (Pre-Flight Health Check)
 */

#ifndef ALG_FUSE_DETECT_H
#define ALG_FUSE_DETECT_H

#include "bms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Resultado da auto-identificação pré-partida (Pre-Flight Check)
 */
typedef struct {
    bool preflight_passed;        /**< true se todos os 20 blocos estão saudáveis */
    uint8_t suspect_block_idx;    /**< Índice do bloco sob suspeita se houver anomalia */
    float max_initial_z_score;    /**< Maior Z-score identificado no repouso/pulso */
    float identified_r0_mohm[BMS_NUM_BLOCKS_SERIES]; /**< Vetor de resistências identificadas [mΩ] */
} PreflightReport_t;

/**
 * @brief Parâmetros de calibração do detector de fusível
 */
typedef struct {
    float lambda;                 /**< Fator de esquecimento exponencial (default: 0.98) */
    float p_init;                 /**< Covariância inicial P0 (default: 1000.0) */
    float delta_i_min_a;          /**< Excitation gate: delta I mínimo para atualizar RLS (default: 1.5 A) */
    float z_suspect_threshold;    /**< Limiar de suspeita (default: 2.0) */
    float z_blown_threshold;      /**< Limiar de confirmação de fusível rompido (default: 2.8) */
    uint16_t confirm_counts;      /**< Amostras consecutivas para confirmação de queima (default: 25) */
} FuseDetectConfig_t;

/**
 * @brief Inicializa o algoritmo de detecção de fusível
 * @param config Parâmetros de calibração (ou NULL para valores padrão de engenharia)
 */
void alg_fuse_detect_init(const FuseDetectConfig_t *config);

/**
 * @brief Executa a auto-identificação pré-partida com pulso de excitação controlado (ex: pré-carga ou teste VCU)
 * @param v_baseline Vetor de tensões dos 20 blocos em repouso [V]
 * @param v_under_pulse Vetor de tensões dos 20 blocos sob corrente de teste [V]
 * @param pulse_current_a Corrente do pulso de teste [A] (ex.: 15.0A a 25.0A)
 * @param out_report Ponteiro para o relatório pré-voo
 * @return true se o acumulador foi aprovado sem fusíveis rompidos
 */
bool alg_fuse_detect_run_preflight(const float v_baseline[BMS_NUM_BLOCKS_SERIES],
                                   const float v_under_pulse[BMS_NUM_BLOCKS_SERIES],
                                   float pulse_current_a,
                                   PreflightReport_t *out_report);

/**
 * @brief Processa uma nova amostra de telemetria dos 20 blocos (chamada a cada dt, ex.: 100 Hz)
 * @param telemetry Ponteiro para a estrutura de telemetria com tensões e correntes medidas
 * @param dt_sec Passo de amostragem no tempo [s]
 */
void alg_fuse_detect_update(BmsPackTelemetry_t *telemetry, float dt_sec);

/**
 * @brief Retorna se há algum fusível confirmado como rompido no acumulador
 * @param out_failed_block_idx Se houver falha, grava o índice do bloco (0 a 19)
 * @return true se houver rompimento confirmado em pelo menos 1 bloco
 */
bool alg_fuse_detect_has_blown_fuse(uint8_t *out_failed_block_idx);

/**
 * @brief Retorna a estimativa atual da média e desvio padrão de R0 entre os 20 blocos
 * @param out_mean Média calculada [Ω]
 * @param out_std Devio padrão calculado [Ω]
 */
void alg_fuse_detect_get_stats(float *out_mean, float *out_std);

#ifdef __cplusplus
}
#endif

#endif /* ALG_FUSE_DETECT_H */
