/**
 * @file bms_mock_battery.h
 * @brief Modelo eletrotérmico Thévenin sintético do acumulador (20s18p) com injeção de falhas
 */

#ifndef BMS_MOCK_BATTERY_H
#define BMS_MOCK_BATTERY_H

#include "bms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuração inicial do acumulador simulado
 */
typedef struct {
    float initial_soc;             /**< SOC inicial (0.0 a 1.0) */
    float ambient_temp_c;          /**< Temperatura ambiente [°C] */
    bool enable_adc_noise;         /**< Se true, adiciona ruído de ±1.5 mV nas medições */
    uint32_t random_seed;          /**< Semente do gerador de ruído */
} MockBatteryConfig_t;

/**
 * @brief Inicializa o modelo de bateria sintética com parâmetros nominais e dispersão
 * @param config Ponteiro para estrutura de configuração
 */
void bms_mock_battery_init(const MockBatteryConfig_t *config);

/**
 * @brief Agenda uma queima de fusível em um bloco específico
 * @param block_idx Índice do bloco (0 a 19)
 * @param event_time_sec Instante de tempo [s] em que o rompimento ocorrerá
 */
void bms_mock_battery_schedule_fuse_blow(uint8_t block_idx, float event_time_sec);

/**
 * @brief Executa 1 passo de integração no tempo da bateria simulada
 * @param dt_sec Passo de integração no tempo [s] (ex.: 0.01s para 100 Hz)
 * @param current_a Corrente aplicada no instante atual [A] (positivo = descarga)
 * @param out_telemetry Ponteiro para onde os dados medidos serão gravados
 */
void bms_mock_battery_step(float dt_sec, float current_a, BmsPackTelemetry_t *out_telemetry);

/**
 * @brief Retorna o valor real (ground truth) de R0 do bloco para comparação no teste SIL
 * @param block_idx Índice do bloco (0 a 19)
 * @return R0 físico em Ohms
 */
float bms_mock_battery_get_true_r0(uint8_t block_idx);

/**
 * @brief Retorna o número real de células ativas do bloco
 * @param block_idx Índice do bloco (0 a 19)
 * @return Quantidade de células conectadas (18 = íntegro, 17 = 1 rompida)
 */
uint8_t bms_mock_battery_get_true_active_cells(uint8_t block_idx);

#ifdef __cplusplus
}
#endif

#endif /* BMS_MOCK_BATTERY_H */
