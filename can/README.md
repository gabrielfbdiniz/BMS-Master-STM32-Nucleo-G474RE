# Matriz de Comunicação CAN — BMS Master (Formula ITA)

Este diretório contém a especificação formal de barramento CAN do **BMS Master** para o protótipo Formula SAE Electric 2025-2026.

## Arquivos

* [`bms_can_matrix.dbc`](bms_can_matrix.dbc): Arquivo oficial no padrão Vector CANdb++ (CAN 2.0B / CAN-FD compatível a 500 kbps).

---

## Tabela Resumo de Mensagens e IDs

| ID (Hex) | ID (Dec) | Nome da Mensagem | Período (ms) | Taxa (Hz) | DLC (Bytes) | Destinatários |
| :---: | :---: | :--- | :---: | :---: | :---: | :--- |
| `0x100` | 256 | **BMS_Heartbeat_Status** | 50 | 20 | 8 | VCU, Telemetria |
| `0x101` | 257 | **BMS_Pack_Metrics** | 20 | 50 | 8 | VCU, Inversor, Telemetria |
| `0x102` | 258 | **BMS_Dynamic_Limits** | 50 | 20 | 8 | VCU, Inversor, Telemetria |
| `0x103` | 259 | **BMS_Extremes_MinMax** | 50 | 20 | 8 | VCU, Telemetria |
| `0x110` | 272 | **BMS_CellVoltages_Group1** (Células 1 a 4) | 100 | 10 | 8 | Telemetria |
| `0x111` | 273 | **BMS_CellVoltages_Group2** (Células 5 a 8) | 100 | 10 | 8 | Telemetria |
| `0x112` | 274 | **BMS_CellVoltages_Group3** (Células 9 a 12) | 100 | 10 | 8 | Telemetria |
| `0x113` | 275 | **BMS_CellVoltages_Group4** (Células 13 a 16) | 100 | 10 | 8 | Telemetria |
| `0x114` | 276 | **BMS_CellVoltages_Group5** (Células 17 a 20) | 100 | 10 | 8 | Telemetria |
| `0x120` | 288 | **BMS_Temperatures** (Slaves 1 a 4) | 100 | 10 | 8 | Telemetria |
| `0x130` | 304 | **BMS_Diagnostics_Fuse** (RLS e Z-score) | 200 | 5 | 8 | VCU, Telemetria |

---

## Ocupação e Carga do Barramento

Considerando um barramento a **500 kbps** com frames CAN padrão (11 bits de ID, ~110 a 130 bits com bit stuffing por frame de 8 bytes):

$$\text{Frames por segundo} = 20 + 50 + 20 + 20 + (5 \times 10) + 10 + 5 = 175\text{ frames/s}$$
$$\text{Taxa de dados média} \approx 175 \times 125\text{ bits} \approx 21{,}875\text{ bps}$$
$$\text{Carga de Barramento gerada pelo BMS} = \frac{21{,}875}{500{,}000} \approx \mathbf{4{,}38\%}$$

Essa ocupação ultrabaixa (< 5%) deixa folga massiva (> 95%) para mensagens de alta taxa do Inversor (1 kHz), da VCU, sensores de suspensão e comandos de torque.
