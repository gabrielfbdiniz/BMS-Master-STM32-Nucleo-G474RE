/**
 * @file bms_types.h
 * @brief Tipos fundamentais e constantes de engenharia do BMS Master V0 (Formula ITA)
 * @details Topologia: 20s18p (20 blocos em série, 18 células em paralelo por bloco)
 *          Compatível com C99 puro (embarcado STM32 e simulação PC).
 */

#ifndef BMS_TYPES_H
#define BMS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* CONSTANTES DA TOPOLOGIA DO ACUMULADOR (20s18p)                             */
/* ========================================================================== */
#define BMS_NUM_BLOCKS_SERIES        (20U)   /**< Quantidade de blocos em série */
#define BMS_CELLS_PER_BLOCK_PARALLEL (18U)   /**< Quantidade de células em paralelo por bloco */
#define BMS_TOTAL_CELLS              (BMS_NUM_BLOCKS_SERIES * BMS_CELLS_PER_BLOCK_PARALLEL) /**< 360 células */

#define BMS_SLAVES_COUNT             (4U)    /**< 4 escravos L99BM114 (5S cada) */
#define BMS_BLOCKS_PER_SLAVE         (5U)    /**< 5 blocos monitorados por escravo */
#define BMS_NTCS_PER_SLAVE           (5U)    /**< 1 NTC por bloco (mínimo FSAE EV4.5.5) */

/* Limites Elétricos dos Blocos (em Volts e Milivolts) */
#define BMS_CELL_V_MAX_MV            (4200.0f) /**< Tensão máxima absoluta (corte de carga) [mV] */
#define BMS_CELL_V_NOM_MV            (3600.0f) /**< Tensão nominal [mV] */
#define BMS_CELL_V_MIN_MV            (2800.0f) /**< Tensão mínima absoluta (corte de descarga) [mV] */
#define BMS_CELL_V_WARN_LOW_MV       (3000.0f) /**< Alerta de tensão baixa [mV] */
#define BMS_CELL_V_WARN_HIGH_MV      (4180.0f) /**< Alerta de sobretensão iminente [mV] */

/* Limites Elétricos do Pack Total (20S) */
#define BMS_PACK_V_MAX_V             (84.0f)   /**< Tensão máxima do pack [V] */
#define BMS_PACK_V_NOM_V             (72.0f)   /**< Tensão nominal do pack [V] */
#define BMS_PACK_V_MIN_V             (56.0f)   /**< Tensão mínima do pack [V] */

/* ========================================================================== */
/* ESPECIFICAÇÃO DA CÉLULA: SAMSUNG INR21700 (ALTA POTÊNCIA FORMULA SAE)      */
/* ========================================================================== */
/* Modelo de Célula: Samsung INR21700-40T (ou 30T)                           */
/* Tensão Nominal: 3.6V | Capacidade: 4.0 Ah (40T) ou 3.0 Ah (30T)           */
/* DC Internal Resistance (IR) Típica @ 25°C: ~15.0 mΩ                       */
/* Em bloco de 18 células em paralelo (18p): R0_bloco ≈ 15.0 / 18 = 0.833 mΩ */
/* ========================================================================== */
#define BMS_SAMSUNG_21700_CELL_MODEL "Samsung INR21700"
#define BMS_SINGLE_CELL_CAPACITY_AH  (4.0f)    /**< Capacidade nominal Samsung 40T: 4.0 Ah (se 30T, use 3.0f) */
#define BMS_BLOCK_NOM_CAPACITY_AH    (BMS_SINGLE_CELL_CAPACITY_AH * BMS_CELLS_PER_BLOCK_PARALLEL) /**< 72.0 Ah */
#define BMS_SINGLE_CELL_R0_OHM       (0.0150f) /**< DC-IR típica da Samsung INR21700 @ 25°C = 15.0 mΩ */
#define BMS_BLOCK_NOM_R0_OHM         (BMS_SINGLE_CELL_R0_OHM / (float)BMS_CELLS_PER_BLOCK_PARALLEL) /**< 0.8333 mΩ */

/* Limites Térmicos (FSAE EV4.5.5) */
#define BMS_CELL_TEMP_MAX_C          (60.0f)   /**< Limite mandatório FSAE: corte a 60 °C */
#define BMS_CELL_TEMP_WARN_C         (55.0f)   /**< Início do derating térmico progressivo [°C] */

/* Limites de Corrente (Carro SEM FREIO REGENERATIVO) */
#define BMS_MAX_DISCHARGE_CURRENT_A  (200.0f)  /**< Pico de aceleração permitido [A] */
#define BMS_CONT_DISCHARGE_CURRENT_A (90.0f)   /**< Corrente contínua de tração [A] */
#define BMS_MAX_CHARGE_CURRENT_A     (0.0f)    /**< Carro sem regeneração em pista (0A) */
#define BMS_CHARGER_BOX_CURRENT_A    (15.0f)   /**< Corrente contínua de recarga em box (lenta) [A] */

/* ========================================================================== */
/* ENUMS DE ESTADO E DIAGNÓSTICO                                             */
/* ========================================================================== */

/**
 * @brief Estados da Máquina Principal de Controle do Acumulador (FSM Master)
 */
typedef enum {
    BMS_STATE_INIT = 0,         /**< Inicialização de hardware, autoteste e calibração */
    BMS_STATE_STANDBY,          /**< Aguardando comando de ativação da VCU (AIRs abertos) */
    BMS_STATE_PRECHARGE,        /**< Pré-carga ativa (AIR- e relé de pré-carga fechados) */
    BMS_STATE_PREFLIGHT_CHECK,  /**< Diagnóstico rápido de impedância pré-partida (Pre-Flight) */
    BMS_STATE_RUN,              /**< Operação normal de tração (AIR+ e AIR- fechados) */
    BMS_STATE_DERATED,          /**< Operação com limite de potência reduzido (falha leve/aviso) */
    BMS_STATE_CHARGING,         /**< Modo de recarga lenta em box */
    BMS_STATE_FAULT             /**< Falha crítica: SDC aberto, AIRs abertos imediatamente */
} BmsMasterState_t;

/**
 * @brief Nível de severidade de diagnóstico do Fusível por Bloco
 */
typedef enum {
    FUSE_STATUS_HEALTHY = 0,    /**< 18 células ativas. Z-score normal (< 2.5) */
    FUSE_STATUS_SUSPECT,        /**< Anomalia detectada (2.5 <= Z-score < 3.5), sob confirmação */
    FUSE_STATUS_BLOWN_CONFIRMED,/**< Rompimento de 1 ou mais fusíveis confirmado (Z >= 3.5) */
    FUSE_STATUS_BLOCK_ISOLATED  /**< Falha catastrófica: bloco aberto (tensão fora de faixa) */
} FuseStatus_t;

/**
 * @brief Flags de falha de segurança para trip do SDC (AMS Trip)
 */
typedef union {
    struct {
        uint32_t over_voltage      : 1; /**< Alguma célula excedeu V_MAX */
        uint32_t under_voltage     : 1; /**< Alguma célula caiu abaixo de V_MIN */
        uint32_t over_temperature   : 1; /**< Alguma célula excedeu 60 °C */
        uint32_t comm_loss_slaves  : 1; /**< Perda de comunicação Iso-SPI com os escravos */
        uint32_t current_sensor_err: 1; /**< Falha no transdutor de corrente (Hall/Shunt) */
        uint32_t precharge_timeout : 1; /**< Falha no transitório de pré-carga */
        uint32_t contactor_welded  : 1; /**< Contato auxiliar indica AIR soldado aberto/fechado */
        uint32_t fuse_blown_severe : 1; /**< Rompimento crítico de fusíveis em múltiplos blocos */
        uint32_t preflight_fuse_blown : 1; /**< Bloco com fusível rompido detectado no auto-teste pré-partida */
        uint32_t reserved          : 23;
    } bits;
    uint32_t raw_flags;
} BmsSafetyFaults_t;

/* ========================================================================== */
/* ESTRUTURAS DE DADOS DE TELEMETRIA E TELEMETRIA SINTÉTICA                  */
/* ========================================================================== */

/**
 * @brief Telemetria medida e variáveis estimadas de um Bloco individual
 */
typedef struct {
    float voltage_v;             /**< Tensão terminal medida no bloco [V] */
    float temperature_c;         /**< Temperatura medida pelo NTC associado [°C] */
    float r0_estimated_ohm;      /**< Resistência interna R0 estimada via RLS [Ω] */
    float z_score;               /**< Desvio padrão relativo aos 20 blocos */
    FuseStatus_t fuse_status;    /**< Estado de saúde do fusível deste bloco */
    uint16_t suspect_counter;    /**< Contador de persistência para evitar falsos positivos */
    uint8_t active_cells;        /**< Quantidade de células ativas estimadas (18 = 100%) */
} CellBlockData_t;

/**
 * @brief Telemetria completa do Acumulador de Tração
 */
typedef struct {
    float pack_voltage_v;        /**< Tensão total do acumulador (soma dos blocos) [V] */
    float pack_current_a;        /**< Corrente total (positivo = descarga, negativo = recarga) [A] */
    float dclink_voltage_v;      /**< Tensão no lado do inversor pós-AIRs [V] */
    
    float soc_percent;           /**< Estado de carga (0.0% a 100.0%) */
    float max_discharge_limit_a; /**< Dynamic Discharge Current Limit (DCL) [A] */
    float max_charge_limit_a;    /**< Dynamic Charge Current Limit (CCL) [A] */
    
    float v_cell_min_v;          /**< Menor tensão entre os 20 blocos [V] */
    float v_cell_max_v;          /**< Maior tensão entre os 20 blocos [V] */
    float temp_cell_max_c;       /**< Maior temperatura registrada [°C] */
    uint8_t min_v_block_idx;     /**< Índice do bloco com menor tensão */
    uint8_t max_v_block_idx;     /**< Índice do bloco com maior tensão */
    uint8_t max_temp_block_idx;  /**< Índice do bloco mais quente */
    
    CellBlockData_t blocks[BMS_NUM_BLOCKS_SERIES]; /**< Dados individuais dos 20 blocos */
    
    BmsMasterState_t state;      /**< Estado operacional da FSM */
    BmsSafetyFaults_t faults;    /**< Registro de falhas de segurança */
    bool sdc_relay_closed;       /**< Estado do relé SDC (true = fechado/seguro, false = aberto) */
} BmsPackTelemetry_t;

#ifdef __cplusplus
}
#endif

#endif /* BMS_TYPES_H */
