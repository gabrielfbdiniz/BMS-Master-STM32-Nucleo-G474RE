# BMS Master — STM32 Nucleo G474RE V0 (Formula ITA)
## ECU Central de Gerenciamento do Acumulador de Tração (20s18p)

Repositório unificado (Monorepo) de **Hardware**, **Firmware** e **Documentação Técnica** do BMS Master do protótipo Formula SAE Electric da equipe **Formula ITA**.

---

## Estrutura do Repositório (Monorepo)

```text
BMS Master - STM - V0/
├── docs/                        # Engenharia de sistemas, especificações e relatórios
│   ├── PLANO_MESTRE_BMS_MASTER_CONSOLIDADO.md  # Especificação técnica completa
│   ├── PLANO_DETECCAO_FUSIVEL_METODO_1.md      # Formulação matemática do RLS e Z-score
│   ├── GUIA_ENSAIO_CARACTERIZACAO_CELULAS_21700.md # Manual de bancada HPPC (FreedomCAR)
│   └── ARQUITETURA_E_PLANEJAMENTO_BMS_MASTER.md
│
├── firmware/                    # Código em C99 Puro (ARM Cortex-M4F & Simulação SIL)
│   ├── core/                    # Algoritmos puros desacoplados de hardware
│   │   ├── bms_types.h          # Topologia 20s18p, limites e telemetria
│   │   ├── alg_fuse_detect.c/.h # RLS incremental, Z-score e Pre-Flight Check
│   │   ├── alg_soc.c/.h         # Coulomb counting, OCV LUT e limites CCL/DCL
│   │   └── fsm_master.c/.h      # Máquina de estados operacional (FSAE EV4.8 / EV8)
│   ├── mock/                    # Motor de sensoriamento sintético de alta fidelidade
│   │   ├── bms_mock_battery.c/.h# Modelo eletrotérmico Thévenin 1RC (Samsung 21700)
│   │   └── drive_cycle.c/.h     # Perfil de corrida FSAE sem regeneração (frenagem mecânica)
│   ├── hal_interface/           # Abstração de hardware (ST HAL / Drivers SPI / FDCAN)
│   ├── tests_sil/               # Runner de Software-in-the-Loop no PC
│   │   ├── main_sil_runner.c    # Simulação de 60s com injeção de falha programada
│   │   ├── sil_runner.exe       # Executável compilado
│   │   └── sil_telemetry_log.csv# Log com 6.000 amostras temporais
│   ├── tools/                   # Scripts de apoio e gráficos
│   │   ├── sim_runner.py        # Simulador interativo em Python
│   │   └── plot_telemetry.py    # Gerador de gráficos da telemetria SIL
│   └── Makefile                 # Build para ARM Cortex-M4F e Clang/GCC Host
│
├── hardware/                    # Projeto Eletrônico no KiCad 10
│   └── carrier_shield/          # Carrier Board de 4 camadas para NUCLEO-G474RE
│       ├── bms_carrier_shield.kicad_pro  # Projeto KiCad
│       ├── bms_carrier_shield.kicad_sch  # Esquemático Mestre Hierárquico (6 blocos)
│       ├── bms_carrier_shield.kicad_pcb  # Layout 4 camadas (Top/GND/PWR/Bot)
│       ├── libraries/           # Símbolos e footprints de componentes
│       └── gerbers/             # Arquivos de fabricação gerados
│
└── simulation/                  # Simulação de circuitos analógicos (Proteus VSM)
    └── proteus_sdc_precharge/   # Circuito biestável SDC e transitório RC de pré-carga
```

---

## Especificações do Sistema

* **Topologia do Acumulador:** 20 blocos em série, cada bloco com 18 células em paralelo (**20s18p = 360 células**).
* **Célula Adotada:** Samsung INR21700 (40T / 30T).
* **Resistência Interna Nominal do Bloco:** $0{,}8333\text{ m}\Omega$ (íntegro 18p) $\to$ $0{,}8824\text{ m}\Omega$ (1 fusível rompido 17p, delta de $+49\ \mu\Omega$).
* **Microcontrolador Central:** ST NUCLEO-G474RE (ARM Cortex-M4F com FPU a 170 MHz).
* **Barreira de Isolação:** Iso-SPI ST `L99BM1TR` + transformador `Pulse HMU2103NL` ($\ge 2500\text{ V}_{RMS}$, creepage $\ge 4{,}0\text{ mm}$).
* **Comunicação Veicular:** FDCAN isolado via transceptor TI `ISO1042` (até 5 Mbps).
* **Segurança FSAE (EV4.8):** Relé SDC normalmente aberto com circuito Latch biestável por hardware.

---

## Como Executar os Testes SIL (PC)

Abra o terminal PowerShell na pasta `firmware/` e execute:

```powershell
# 1. Compilar e executar a simulação SIL nativa em C
clang -O2 -Wall -Icore -Imock tests_sil/main_sil_runner.c core/alg_fuse_detect.c core/alg_soc.c core/fsm_master.c mock/bms_mock_battery.c mock/drive_cycle.c -o tests_sil/sil_runner.exe -lm
.\tests_sil\sil_runner.exe

# 2. Gerar os gráficos de telemetria
python tools/plot_telemetry.py
```

---

## Como Abrir o Projeto Eletrônico no KiCad 10

1. Abra o **KiCad 10**.
2. Clique em **Abrir Projeto** e selecione o arquivo:  
   `hardware/carrier_shield/bms_carrier_shield.kicad_pro`.
3. Navegue entre as folhas hierárquicas:
   - `01_power_supply.kicad_sch`: Regulador automotivo GLV 12V $\to$ 5V com proteções ISO 7637-2.
   - `02_isospi_barrier.kicad_sch`: Subsistema Iso-SPI L99BM1TR com transformador de pulso 2500V.
   - `03_sdc_latch.kicad_sch`: Circuito Latch biestável do relé SDC (FSAE EV4.8).
   - `04_contactors_drivers.kicad_sch`: High-Side Drivers automotivos de AIR+, AIR- e Pré-Carga.
   - `05_canfd_interface.kicad_sch`: Transceptor FDCAN isolado ISO1042.
   - `06_nucleo_headers.kicad_sch`: Conectores Morpho de encaixe da NUCLEO-G474RE.

---

*Formula ITA — Temporada 2025-2026.*
