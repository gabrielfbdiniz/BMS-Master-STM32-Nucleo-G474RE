import re

with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
    text = f.read()

print("Latch components locations:")
for ref in ['U4', 'K1', 'Q2', 'Q3', 'SW1', 'D5', 'D6']:
    m = re.search(r'property "Reference"\s+"' + ref + r'"[^(]+\(at\s+([0-9.-]+)\s+([0-9.-]+)', text)
    if not m:
        # try finding footprint
        lines = text.split('(footprint ')
        for fp in lines:
            if f'property "Reference" "{ref}"' in fp:
                m_at = re.search(r'\(at\s+([0-9.-]+)\s+([0-9.-]+)', fp)
                if m_at:
                    print(f"  {ref:3s}: at ({float(m_at.group(1)):6.2f}, {float(m_at.group(2)):6.2f})")
    else:
        print(f"  {ref:3s}: at ({float(m.group(1)):6.2f}, {float(m.group(2)):6.2f})")
