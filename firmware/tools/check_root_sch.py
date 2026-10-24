import re

with open('hardware/carrier_shield/bms_carrier_shield.kicad_sch', 'r', encoding='utf-8') as f:
    text = f.read()

sheets = re.findall(r'\(property "Sheetname" "([^"]+)"', text)
print('Sheets in root:', sheets)

pins = re.findall(r'\(pin "([^"]+)"', text)
print(f'Total sheet pins in root: {len(pins)}')
for p in pins[:30]:
    print('  Pin:', p)

wires = re.findall(r'\(wire\s+\(pts\s+\(xy\s+([0-9.-]+)\s+([0-9.-]+)\)\s+\(xy\s+([0-9.-]+)\s+([0-9.-]+)\)\)', text)
print(f'Total wires in root: {len(wires)}')
