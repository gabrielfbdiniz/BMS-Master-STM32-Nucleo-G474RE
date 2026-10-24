/**
 * @file fsm_master.h
 * @brief Máquina de Estados Operacional Principal do BMS Master (FSAE EV4.8 / EV8)
 */

#ifndef FSM_MASTER_H
#define FSM_MASTER_H

#include "bms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa a máquina de estados do BMS Master
 * @param telemetry Ponteiro para a estrutura de telemetria
 */
void fsm_master_init(BmsPackTelemetry_t *telemetry);

/**
 * @brief Solicita fechamento de contatores para iniciar corrida (comando de Pré-Carga)
 */
void fsm_master_request_run(void);

/**
 * @brief Solicita abertura suave de contatores (retorno a Standby)
 */
void fsm_master_request_standby(void);

/**
 * @brief Executa 1 ciclo da FSM principal (chamada a 100 Hz)
 * @param telemetry Ponteiro para a estrutura de telemetria
 * @param dt_sec Passo de tempo [s]
 */
void fsm_master_update(BmsPackTelemetry_t *telemetry, float dt_sec);

#ifdef __cplusplus
}
#endif

#endif /* FSM_MASTER_H */
