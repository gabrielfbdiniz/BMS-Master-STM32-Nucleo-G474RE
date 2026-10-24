import re

with open('hardware/carrier_shield/06_nucleo_headers.kicad_sch', 'r', encoding='utf-8') as f:
    text = f.read()

labels = re.findall(r'\((?:hierarchical_label|label)\s+"([^"]+)"', text)
print("All labels in 06_nucleo_headers.kicad_sch:")
for l in sorted(set(labels)):
    print(f"  {l}")

# Let's inspect J1 pin by pin
from parse_headers import parse_header_pads
headers = parse_header_pads('hardware/carrier_shield/bms_carrier_shield.kicad_pcb')
print("\n--- J1 Pins ---")
j1 = headers['J1']['pads']
for p in range(1, 39):
    print(f"Pin {p:2d}: {j1[p]['net']}")

print("\n--- J2 Pins ---")
j2 = headers['J2']['pads']
for p in range(1, 39):
    print(f"Pin {p:2d}: {j2[p]['net']}")
