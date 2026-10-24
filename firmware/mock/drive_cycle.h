/**
 * @file drive_cycle.h
 * @brief Gerador de perfil dinâmico de corrente de corrida (Formula SAE Drive Cycle)
 */

#ifndef DRIVE_CYCLE_H
#define DRIVE_CYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o gerador de perfil de corrida
 */
void drive_cycle_init(void);

/**
 * @brief Retorna a corrente de tração instantânea [A] para o instante t_sec
 * @param t_sec Tempo decorrido em segundos desde o início da prova
 * @return Corrente em Amperes (positivo = descarga do pack, negativo = regeneração)
 */
float drive_cycle_get_current(float t_sec);

#ifdef __cplusplus
}
#endif

#endif /* DRIVE_CYCLE_H */
