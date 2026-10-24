import re

with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
    text = f.read()

lines = text.split('(footprint ')
for ref in ['U4', 'K1', 'Q2', 'Q3', 'SW1', 'D5', 'D6', 'J5', 'J1', 'J2']:
    for fp in lines[1:]:
        if f'property "Reference" "{ref}"' in fp:
            m_at = re.search(r'\(at\s+([0-9.-]+)\s+([0-9.-]+)', fp)
            if m_at:
                print(f"  {ref:4s}: X={float(m_at.group(1)):6.2f}, Y={float(m_at.group(2)):6.2f}")
            break
