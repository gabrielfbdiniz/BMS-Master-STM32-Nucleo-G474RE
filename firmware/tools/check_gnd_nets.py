import re

with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
    text = f.read()

print("Pads with GND in J1 and J2:")
# let's find pads in J1 and J2 that have GND
for j in ['J1', 'J2']:
    print(f"\n--- {j} ---")
    lines = text.split('(footprint ')
    for fp in lines:
        if f'property "Reference" "{j}"' in fp:
            for pad_line in fp.split('(pad '):
                if 'thru_hole' in pad_line:
                    p_num = pad_line.split('"')[1]
                    m_net = re.search(r'\(net\s+"([^"]+)"\)', pad_line)
                    net = m_net.group(1) if m_net else "NO_NET"
                    if 'GND' in net or net == 'GND':
                        print(f"  Pin {p_num:2s}: {net}")
