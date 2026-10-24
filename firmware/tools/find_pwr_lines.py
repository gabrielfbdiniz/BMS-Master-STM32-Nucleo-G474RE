with open('hardware/carrier_shield/06_nucleo_headers.kicad_sch', 'r', encoding='utf-8') as f:
    lines = f.readlines()

for idx, line in enumerate(lines):
    if any(k in line for k in ['+5V', '+3V3', 'GND', 'VDD', 'E5V', 'VIN', 'AVDD', 'AGND', 'U5V', 'IOREF', 'VBAT']):
        print(f"Line {idx+1:4d}: {line.strip()}")
