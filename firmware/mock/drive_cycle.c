/**
 * @file drive_cycle.c
 * @brief Implementação do perfil de corrente de endurance/autocross FSAE
 */

#include "drive_cycle.h"
#include <math.h>

void drive_cycle_init(void)
{
    /* Sem necessidade de alocação dinâmica */
}

float drive_cycle_get_current(float t_sec)
{
    /* Período de uma volta típica de autocross/endurance: ~45 segundos */
    float t_lap = fmodf(t_sec, 45.0f);
    float current = 0.0f;

    if (t_lap < 2.0f) {
        /* Saída de curva lenta / Início da volta: aceleração suave */
        current = 30.0f + 25.0f * (t_lap / 2.0f);
    } 
    else if (t_lap < 5.5f) {
        /* Reta Principal: Aceleração a fundo (Pé embaixo até 175 A) */
        float tau = (t_lap - 2.0f) / 3.5f;
        current = 140.0f + 35.0f * sinf(tau * 3.14159f * 0.5f);
    }
    else if (t_lap < 7.5f) {
        /* Fim da reta: Frenagem mecânica pura (SEM REGEN) */
        /* Corrente cai para consumo dos sistemas auxiliares (bombas, ventoinhas, LV) */
        current = 2.0f;
    }
    else if (t_lap < 12.0f) {
        /* Sequência de curvas sinuosas (Chicane): modulação de acelerador */
        current = 45.0f + 25.0f * sinf((t_lap - 7.5f) * 2.5f);
    }
    else if (t_lap < 14.0f) {
        /* Pequena reta intermediária */
        current = 130.0f;
    }
    else if (t_lap < 16.0f) {
        /* Frenagem mecânica pura */
        current = 2.0f;
    }
    else if (t_lap < 24.0f) {
        /* Setor de baixa velocidade (Hairpin + Slalom): acelera e freia mecânico */
        float sub = fmodf(t_lap - 16.0f, 2.0f);
        if (sub < 1.2f) {
            current = 85.0f;
        } else {
            current = 2.5f; /* Sem corrente negativa */
        }
    }
    else if (t_lap < 28.0f) {
        /* Reta oposta: Aceleração forte */
        current = 165.0f;
    }
    else if (t_lap < 31.0f) {
        /* Frenagem mecânica pura */
        current = 2.0f;
    }
    else if (t_lap < 40.0f) {
        /* Miolo do circuito: oscilação contínua entre 50A e 110A */
        current = 75.0f + 35.0f * cosf((t_lap - 31.0f) * 1.5f);
    }
    else {
        /* Curva final e preparação para a reta */
        current = 40.0f;
    }

    /* Adiciona uma pequena ondulação de alta frequência para simular chaveamento do inversor PWM */
    float ripple = 1.8f * sinf(t_sec * 62.83f); /* ~10 Hz */
    return current + ripple;
}
