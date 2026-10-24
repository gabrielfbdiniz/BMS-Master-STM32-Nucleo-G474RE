# 🏎️ RELATÓRIO DE AUDITORIA TÉCNICA & PARECER DE JULGAMENTO — BMS MASTER V0
## Avaliação de Engenharia Sênior & Inspeção Técnica Formula SAE Electric (FSAE EV)
**Equipe:** Formula ITA  
**Sistema:** Accumulator Management System (AMS) / BMS Master — NUCLEO-G474RE Carrier Shield  
**Data da Auditoria:** 23 de Outubro de 2026  
**Status no Escrutínio Técnico:** 🛑 **REPROVADO / INSPECTION FAILED (Pendências Críticas Bloqueantes)**  

---

## 1. 📋 Veredito Executivo do Juiz (Executive Summary)

Como **Juiz Técnico de Design e Escrutínio Elétrico de Formula SAE** e **Engenheiro Sênior de Sistemas Embarcados Automotivos**, realizei a varredura completa de cada arquivo do repositório: folhas de esquemático KiCad 10, layout da placa 2D/3D (`.kicad_pcb`), algoritmos de controle em C99, ambiente de teste SIL e documentação de engenharia.

### Diagnóstico Direto:
O projeto possui uma **concepção teórica e algorítmica de altíssimo nível** (destaque internacional para a modelagem do RLS com Z-Score para detecção de fusíveis rompidos em 18p e o ambiente SIL sintético). Contudo, o **desenvolvimento do hardware eletrônico está em um estado embrionário e com erros conceituais graves de EDA (KiCad)** que impediriam o funcionamento da placa caso fosse enviada à fabricação hoje.

> **🛑 ERRO BLOQUEANTE NÚMERO 1: ISOLAMENTO TOTAL ENTRE FOLHAS NO KICAD**  
> No esquemático hierárquico `bms_carrier_shield.kicad_sch`, os blocos de folha (`sheet`) **não possuem pinos hierárquicos desenhados**. Consequentemente, o compilador de netlist do KiCad tratou os sinais das folhas filhas como nós isolados locais (ex: gerando `/02_IsoSPI_Galvanic_Barrier/PB3_SPI1_SCK` de um lado e `/05_Nucleo_G474RE_Morpho_Headers/PB3_SPI1_SCK` do outro).  
> **Na PCB atual, o transceptor Iso-SPI, a interface CAN, o Latch do SDC e os conectores da NUCLEO NÃO POSSUEM CONEXÃO ELÉTRICA ENTRE SI.**

> **⚠️ ERRO BLOQUEANTE NÚMERO 2: FOLHA DE CONTATORES ESQUECIDA**  
> A folha `04_contactors_drivers.kicad_sch` (drivers dos AIRs e Pré-carga) existe na pasta, mas **NEM SEQUER FOI INSTANCIADA no esquemático raiz nem incluída na PCB**. Os transistores de acionamento das bobinas dos contatores não existem no layout.

---

## 2. 🌟 O que o Projeto tem de Bom (Pontos Fortes)

1. **Escolha da Plataforma de Controle (NUCLEO-G474RE):**
   - Decisão acertadíssima de engenharia: microcontrolador automotivo ARM Cortex-M4F rodando a 170 MHz com FPU, CORDIC e FMAC, 3x controladores FDCAN e ADCs rápidos. Reduz o risco de solda e tempo de bancada para quase zero com a estratégia Carrier Shield.
2. **Arquitetura de Barreira Iso-SPI Robusta:**
   - Adoção da família de BMS automotivo da ST: CI transceptor dedicado **L99BM1TR** casado com o transformador de pulso automotivo de isolação reforçada **Pulse HMU2103NL** ($\ge 2500\text{ V}_{RMS}$, creepage $\ge 4{,}0\text{ mm}$). Elimina problemas de ruído e correntes de fuga de optoacopladores lentos.
3. **Formulação Matemática e Simulação SIL (Software-in-the-Loop):**
   - A modelagem matemática em `PLANO_DETECCAO_FUSIVEL_METODO_1.md` e a implementação em `alg_fuse_detect.c` com RLS escalar incremental excitado por $dI/dt$ e teste Z-score inter-blocos é exemplar. O runner SIL em C99 puro gera 6.000 amostras reproduzíveis e gráficos de telemetria sem depender de hardware físico.
4. **Padronização Monorepo & Documentação:**
   - Excelente organização documental no `PLANO_MESTRE_BMS_MASTER_CONSOLIDADO.md`, alinhando requisitos mecânicos, elétricos e de software.

---

## 3. 🚨 O que temos de Ruim e Incompleto (Varredura Detalhada)

### 3.1. Esquemático Eletrônico (KiCad)

| Item Auditado | Arquivo | Diagnóstico Técnico | Impacto |
| :--- | :--- | :--- | :--- |
| **Hierarquia de Folhas** | `bms_carrier_shield.kicad_sch` | Os blocos `sheet` não possuem pinos de conexão (`sheet_pin`). As `hierarchical_label` nas folhas filhas tornam-se nós locais desconectados. | **Fatal:** Microcontrolador não se comunica com nenhum chip da placa. |
| **Drivers de Contatores** | `04_contactors_drivers.kicad_sch` | A folha existe fisicamente mas **não está instanciada** no esquemático mestre. | **Fatal:** O veículo não possui comando para ligar AIR+, AIR- ou Pré-carga. |
| **Alimentação dos AIRs vs SDC** | `04_contactors_drivers.kicad_sch` | A alimentação das bobinas dos contatores parece vir de `+12V` geral da baixa tensão. | **Violação FSAE EV5.1:** As bobinas dos AIRs **devem** ser desenergizadas diretamente pelo laço físico do SDC. |
| **Topologia dos Drivers** | `04_contactors_drivers.kicad_sch` | Usa MOSFET discreto canal P `IRLML6402` (SOT-23) + `2N7002`. Bobinas de contatores industriais (Gigavac/Kilovac) demandam $1.5\text{ A}$ a $3\text{ A}$ no inrush. | **Risco de Queima:** Falta de proteção contra curto no chicote, sobretemperatura e limitação de corrente. |
| **Sensoriamento de Alta Tensão (HV)** | `06_nucleo_headers.kicad_sch` | Os pinos `PA1_HV_PACK` e `PA2_HV_DCLINK` estão conectados diretamente ao barramento de pinos do microcontrolador sem amplificador isolado (`AMC1311`), sem divisor resistivo e sem clamp zener! | **Destruição Imediata:** Se $84\text{ V}$ entrarem no pino da Nucleo ($3.3\text{ V}$ máx), o microcontrolador é incinerado. |
| **Interface do Sensor LEM** | `06_nucleo_headers.kicad_sch` | Labels `PA6_CURR_LEM1` e `PA7_CURR_LEM2` sem conector de entrada, sem filtro passa-baixas e sem proteção de sobretensão. | **Incompleto:** Não há como conectar os sensores de corrente do pack. |
| **Relé do SDC (Omron G5V-1)** | `03_sdc_latch.kicad_sch` | O relé `G5V-1` suporta corrente de comutação máxima de **1 A DC**. Se o laço de desligamento do carro alimentar múltiplas bobinas de relés de segurança e contatores, a corrente de inrush soldará os contatos do relé. | **Risco de Falha de Segurança:** Recomenda-se relé automotivo de no mínimo 3 A a 5 A ou relé de sinal robusto classe telecom. |

---

### 3.2. Layout da Placa de Circuito Impresso 2D (`bms_carrier_shield.kicad_pcb`)

1. **Roteamento Extremamente Incompleto:**
   - A placa possui **288 pads** e apenas **236 segmentos de trilha** e **4 vias**! Mais de 80% dos sinais estão em aberto.
   - Os barramentos de alimentação de `+5V`, `+3V3` e planos internos `In1.Cu` (GND) e `In2.Cu` (PWR) não possuem vias de interligação distribuídas adequadamente para os CIs.
2. **Barreira Galvânica e Creepage/Clearance (Regra EV 4.4):**
   - O transformador de pulso `HMU2103NL` está posicionado, porém não há **rasgo mecânico de isolação (milling slot)** desenhado na camada `Edge.Cuts` sob o transformador.
   - O keepout desenhado é apenas um polígono genérico que não garante conformidade visual no escrutínio para os 4,0 mm de creepage exigidos pelo comitê.
3. **Ausência de Conectores Críticos:**
   - A placa tem apenas os conectores `J3` (GLV 12V), `J4` (Iso-SPI), `J5` (SDC) e `J6` (CAN).
   - **Faltam:** Conector para as bobinas dos AIRs (`AIR+`, `AIR-`, `Precharge`), conector para feedback dos contatos auxiliares, conector para leitura da alta tensão do barramento e conector para o sensor de corrente Hall analógico.

---

### 3.3. Firmware & Arquitetura de Software

1. **Camada de Abstração de Hardware Inexistente (`hal_interface/` vazia):**
   - O diretório `firmware/hal_interface/` está com 0 arquivos.
   - Não há driver SPI implementado para o chip **L99BM1TR**.
   - Não há parser dos quadros Iso-SPI para ler e configurar os 4 escravos **L99BM114**.
   - Não há driver de periférico **FDCAN** para transmitir a telemetria à VCU e receber mensagens do inversor ou sensor de corrente.
   - Não há rotinas de interrupção nem leitura de ADC via DMA.
2. **Sensibilidade Real do Algoritmo de Fusível (RLS em Pista):**
   - A fórmula $R_0 = \Delta V / \Delta I$ assume medições rigorosamente síncronas. Como o L99BM114 mede via daisy-chain e o sensor de corrente mede via CAN ou ADC, qualquer jitter temporal de $\sim 5\text{ ms}$ entre as duas amostras durante aceleração máxima ($dI/dt > 1000\text{ A/s}$) gerará erros que o Z-score interpretará como falsa queima de fusível.
   - O algoritmo não possui tabela de correção de impedância em função da temperatura ($R_0(T)$). Um desvio térmico natural de apenas $2{,}5\ ^\circ\text{C}$ entre blocos causa uma variação de resistência similar à queima de um fusível ($+5{,}88\%$).

---

### 3.4. Simulação Analógica (`simulation/` vazia)

- A pasta `simulation/proteus_sdc_precharge/` está vazia. O circuito de travamento biestável com flip-flop 74HC74 e o comportamento do transistor sob transitórios de partida nunca foram validados em arquivo de simulação persistido.

---

## 4. ⚖️ Parecer do Juiz de Competição (FSAE Rules Checklist)

| Regra FSAE EV | Descrição do Regulamento | Conformidade Atual | Observações do Juiz |
| :--- | :--- | :---: | :--- |
| **EV 4.8.1 / EV 8.2** | AMS monitora tensão e temperatura de 100% das células e abre o SDC sob violação. | 🟡 **Parcial** | Lógica implementada no SIL, mas o hardware está desconectado na PCB. |
| **EV 4.8.4** | Trava de falha por hardware (Latch) com reset manual não disponível ao piloto em movimento. | 🟡 **Parcial** | Esquemático possui botão `SW1` na placa, mas não há conexão para botão de reset externo no painel de box. |
| **EV 4.8.5** | O AMS deve ser imune a interferências eletromagnéticas (EMI de inversores e motores). | 🔴 **Não Aprovado** | A PCB não possui planos contínuos de terra roteados nem desacoplamento completo. |
| **EV 4.4.1** | Distâncias de isolação galvânica (Creepage e Clearance) entre circuitos HV e GLV. | 🟡 **Pendente** | Transformador é adequado, mas o layout da PCB não possui cotas nem slots fresados garantindo a separação. |
| **EV 5.1.3** | Os AIRs devem ser alimentados diretamente pelo laço do Shutdown Circuit (SDC). | 🔴 **Não Aprovado** | O circuito de acionamento não passa a alimentação dos contatores pelo contato do SDC. |
| **EV 5.2.1** | Circuito de Pré-Carga com detecção de 95% de $V_{pack}$ antes do fechamento do segundo AIR. | 🔴 **Não Aprovado** | O hardware não possui circuito divisor/isolador de medição analógica de $V_{dclink}$ e $V_{pack}$. |
| **EV 4.5.5** | Monitoramento de temperatura com pelo menos 20% das células cobertas por NTCs. | 🟢 **Conforme (Conceito)** | Topologia prevê 1 NTC por bloco (20 sensores para 20 blocos = 100% de cobertura). |

---

## 5. 🛠️ Plano de Ação para Alcançar a Maturidade Técnica (Roadmap)

1. **Passo 1: Corrigir Imediatamente o Esquemático KiCad (Prioridade Máxima)**
   - Abrir `bms_carrier_shield.kicad_sch` e instanciar os pinos hierárquicos em cada folha (`Sheet Pins`), interligando com fios os sinais de SPI, CAN, Latch SDC e I/Os da NUCLEO.
   - Adicionar o bloco da folha `04_contactors_drivers.kicad_sch` na folha raiz.
   - Substituir rótulos locais por conexões hierárquicas explícitas em todas as folhas.
2. **Passo 2: Adequar os Circuitos de Potência e Sensoriamento**
   - **Drivers de Contatores:** Substituir MOSFETs discretos `IRLML6402` por CIs High-Side protegidos automotivos (ex: `TPS2H160-Q1` ou `VN7020AJ`).
   - **Alimentação dos AIRs:** Garantir que a linha de $12\text{ V}$ dos contatores seja derivada após o contato do relé SDC.
   - **Condicionamento HV:** Criar o circuito com divisores resistivos balanceados e amplificadores de isolação galvânica (`AMC1311B`) para medir $V_{pack}$ e $V_{dclink}$.
   - **Interface de Corrente:** Inserir conector com filtro e proteção para o sensor Hall LEM.
3. **Passo 3: Concluir o Layout da PCB 4 Camadas**
   - Rotear 100% dos sinais do ratsnest.
   - Criar ranhura mecânica (*milling slot* de $1{,}5\text{ mm}$) na camada `Edge.Cuts` sob o transformador `HMU2103NL`.
   - Conectar os planos internos `In1.Cu` (GND) e `In2.Cu` (+5V) com vias térmicas.
4. **Passo 4: Firmware & Drivers de Hardware**
   - Desenvolver em `firmware/hal_interface/` os drivers reais do `L99BM1TR`, `FDCAN` e `ADC`.
   - Implementar tabela de correção térmica $R_0(T)$ para evitar falso diagnóstico de fusível sob gradiente de temperatura.
