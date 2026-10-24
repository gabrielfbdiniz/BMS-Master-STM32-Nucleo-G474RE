/**
 * @file alg_soc.h
 * @brief Algoritmo de cálculo de SOC (Coulomb Counting + OCV LUT) e Limites Dinâmicos (CCL/DCL)
 */

#ifndef ALG_SOC_H
#define ALG_SOC_H

#include "bms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o estimador de SOC com base na tensão de circuito aberto inicial
 * @param telemetry Telemetria do pack com tensões lidas em repouso
 */
void alg_soc_init(BmsPackTelemetry_t *telemetry);

/**
 * @brief Atualiza a estimativa de SOC e calcula DCL / CCL dinâmicos
 * @param telemetry Telemetria do pack a ser atualizada
 * @param dt_sec Passo de tempo [s]
 */
void alg_soc_update(BmsPackTelemetry_t *telemetry, float dt_sec);

#ifdef __cplusplus
}
#endif

#endif /* ALG_SOC_H */
