# 🏭 Especificações de Fabricação e Montagem (Fabrication Notes)
## BMS Master Carrier Shield V0 — Formula ITA (Temporada 2025-2026)

Este documento reúne todas as especificações técnicas, parâmetros de empilhamento (*stackup*), requisitos de isolação galvânica e instruções para cotação e manufatura da placa **BMS Master Carrier Shield V0** em fabricantes industriais de PCB (ex.: JLCPCB, PCBWay, Eurocircuits).

---

## 📦 Arquivos do Pacote de Produção

O pacote pronto para envio à fábrica está localizado em:
`hardware/carrier_shield/gerbers/BMS_Carrier_Shield_Gerbers_v0.zip`

### Conteúdo do Arquivo ZIP:

| Nome do Arquivo | Função / Camada | Formato / Extensão |
| :--- | :--- | :--- |
| `bms_carrier_shield-F_Cu.gtl` | Camada de Cobre Superior (Top Layer) | Gerber RS-274X (.gtl) |
| `bms_carrier_shield-GND_Plane.g1` | Camada Interna 1: Plano de Terra (GND Plane) | Gerber RS-274X (.g1) |
| `bms_carrier_shield-PWR_Plane.g2` | Camada Interna 2: Planos de Alimentação (+5V / +3.3V) | Gerber RS-274X (.g2) |
| `bms_carrier_shield-B_Cu.gbl` | Camada de Cobre Inferior (Bottom Layer) | Gerber RS-274X (.gbl) |
| `bms_carrier_shield-F_Mask.gts` | Máscara de Solda Superior (Top Solder Mask) | Gerber RS-274X (.gts) |
| `bms_carrier_shield-B_Mask.gbs` | Máscara de Solda Inferior (Bottom Solder Mask) | Gerber RS-274X (.gbs) |
| `bms_carrier_shield-F_Silkscreen.gto` | Silkscreen Superior (Top Legend) | Gerber RS-274X (.gto) |
| `bms_carrier_shield-B_Silkscreen.gbo` | Silkscreen Inferior (Bottom Legend) | Gerber RS-274X (.gbo) |
| `bms_carrier_shield-F_Paste.gtp` | Máscara de Pasta Superior (Top Stencil Paste) | Gerber RS-274X (.gtp) |
| `bms_carrier_shield-B_Paste.gbp` | Máscara de Pasta Inferior (Bottom Stencil Paste) | Gerber RS-274X (.gbp) |
| `bms_carrier_shield-Edge_Cuts.gm1` | Contorno Mecânico e Rasgos de Isolação (Board Outline) | Gerber RS-274X (.gm1) |
| `bms_carrier_shield-PTH.drl` | Furos Metalizados (Plated Through-Hole) | Excellon NC Drill (.drl) |
| `bms_carrier_shield-NPTH.drl` | Furos Não-Metalizados (Non-Plated Through-Hole) | Excellon NC Drill (.drl) |

### Arquivos para Montagem Automática (SMT / PCBA):
* **Lista de Materiais (BOM):** [`BMS_Carrier_Shield_BOM_Detailed.csv`](gerbers/BMS_Carrier_Shield_BOM_Detailed.csv) e versão legível [`BMS_Carrier_Shield_BOM_Detailed.md`](gerbers/BMS_Carrier_Shield_BOM_Detailed.md).
* **Posicionamento de Componentes (Pick & Place):** [`bms_carrier_shield-pos.csv`](gerbers/bms_carrier_shield-pos.csv) (coordenadas X, Y, rotação e face de montagem).

---

## ⚙️ Parâmetros Recomendados para Cotação (Order Specifications)

| Parâmetro | Valor de Projeto | Justificativa de Engenharia |
| :--- | :--- | :--- |
| **Número de Camadas** | **4 Camadas** (Multi-layer) | Plano contínuo de terra e plano de alimentação interno dedicados. |
| **Material Base** | **FR-4 Standard ou TG150/TG170** | Maior estabilidade térmica sob vibração e temperatura veicular. |
| **Espessura da Placa** | **1,6 mm ± 10%** | Rigidez mecânica adequada para fixação dos conectores Morpho da Nucleo. |
| **Espessura do Cobre** | **1 oz (35 µm)** externa / **1 oz (35 µm)** interna | Capacidade de corrente das trilhas de alimentação e relé SDC. |
| **Acabamento Superficial** | **ENIG (Electroless Nickel Immersion Gold)** ou **HASL Lead-Free** | Planicidade para soldagem dos pads térmicos HTSOP-8 e SOIC. |
| **Cor da Máscara de Solda** | **Preto** ou **Verde** | Preferência visual da equipe / Alto contraste com a serigrafia branca. |
| **Cor da Serigrafia (Silkscreen)** | **Branco** | Alta legibilidade para debug e identificação dos conectores. |
| **Menor Trilha / Espaçamento** | **0,15 mm / 0,15 mm (6 mil / 6 mil)** | Roteamento padrão de alta produtividade sem custo extra. |
| **Menor Diâmetro de Furo (Via)** | **0,3 mm (furo) / 0,6 mm (pad)** | Vias de sinal e costura térmica do regulador LMR36510. |
| **Castellated Holes / Impedância** | **Não** | Não aplicável nesta versão. |

---

## 📐 Estrutura de Empilhamento das Camadas (Layer Stackup)

```text
┌─────────────────────────────────────────────────────────────┐
│ Camada 1 (Top):     Sinais, Componentes SMD, Vias Térmicas  │  (1 oz / 35 µm)
├─────────────────────────────────────────────────────────────┤  Prepreg (ex.: 2116 / 7628)
│ Camada 2 (In1):     Plano Sólido de Terra (GND)             │  (1 oz / 35 µm)
├─────────────────────────────────────────────────────────────┤  Core FR-4 (1.0 mm)
│ Camada 3 (In2):     Planos de Potência Divididos (+5V / +3.3V)│ (1 oz / 35 µm)
├─────────────────────────────────────────────────────────────┤  Prepreg (ex.: 2116 / 7628)
│ Camada 4 (Bottom):  Sinais de Roteamento Secundários        │  (1 oz / 35 µm)
└─────────────────────────────────────────────────────────────┘
Espessura Total: ~1.6 mm
```

---

## ⚡ Requisitos Críticos de Isolação Galvânica (FSAE EV4.8 / EV8)

1. **Barreira Iso-SPI (Transformador Pulse HMU2103NL):**
   - Distância de *creepage* e *clearance* entre o lado GLV (GND) e o lado de alta tensão (GND_ISO) $\ge 4{,}0\text{ mm}$ contínuos em todas as 4 camadas.
   - Nenhuma trilha de sinal ou cobre cruza sob o corpo do transformador `T1`.
2. **Barreira CAN-FD (TI ISO1044 + Mornsun B0505S-1WR3):**
   - Isolação de até 5000 Vrms com separação física entre o domínio GLV do veículo e o barramento externo de telemetria.
3. **Malha do Shutdown Circuit (SDC Latch):**
   - Contatos secos do relé Omron `G5V-1-DC5` operando estritamente na malha de 12V GLV.

---

## 🪛 Instruções para Montagem Manual em Laboratório (Bench Assembly)

Caso a montagem seja feita manualmente pela equipe:
1. **Ordem de Soldagem Recomendada:**
   1. Circuitos integrados SMD menores (U3 USBLC6, D8 NUP2105L, Q2, Q3).
   2. Resistores e capacitores 0603 e 0805.
   3. CIs SOIC e HTSOP (U1 LMR36510 com pad térmico inferior bem estanhado, U2 L99BM1, U4 74HC74, U5 ISO1044).
   4. Transformador de pulso SMD T1 `HMU2103NL`.
   5. Indutores de potência L1 e L2 (`SRR1260`).
   6. Capacitor eletrolítico C1 e LEDs de status.
   7. Componentes passantes (THT): Módulo conversor DC-DC U6 `B0505S-1WR3`, relé K1 `G5V-1`, botão SW1 e conectores Molex Micro-Fit.
   8. Barras de pinos fêmea Morpho 2x19 (J1 e J2) para encaixe da NUCLEO-G474RE.
2. **Inspeção Pré-Energização:**
   - Realizar teste de continuidade com multímetro entre `+12V` e `GND`, `+5V` e `GND`, `+3.3V` e `GND` para garantir que não há curtos antes de ligar a fonte DC.
