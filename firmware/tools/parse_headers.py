import re

def parse_header_pads(pcb_path):
    with open(pcb_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Find footprints
    fp_blocks = text.split('(footprint ')
    headers = {}

    for fp in fp_blocks[1:]:
        m_ref = re.search(r'property "Reference"\s+"(J[12])"', fp)
        if not m_ref:
            continue
        ref = m_ref.group(1)
        
        m_at = re.search(r'\(at\s+([0-9.-]+)\s+([0-9.-]+)(?:\s+([0-9.-]+))?\)', fp)
        fp_x = float(m_at.group(1))
        fp_y = float(m_at.group(2))
        fp_rot = float(m_at.group(3)) if m_at.group(3) else 0.0
        
        # Parse all pads
        # Pattern: (pad "(\d+)" thru_hole (?:circle|rect)\s+\(at ([0-9.-]+) ([0-9.-]+)...\s+\(net "([^"]+)"\)
        # Pads in KiCad 10:
        pad_blocks = re.findall(r'\(pad\s+"(\d+)"\s+thru_hole\s+(?:circle|rect)\s+\(at\s+([0-9.-]+)\s+([0-9.-]+)[^)]*\).*?\(net\s+"([^"]*)"\)', fp, re.DOTALL)
        
        pads_dict = {}
        for pb in pad_blocks:
            p_num = int(pb[0])
            rel_x = float(pb[1])
            rel_y = float(pb[2])
            net = pb[3]
            abs_x = fp_x + rel_x
            abs_y = fp_y + rel_y
            pads_dict[p_num] = {
                'rel': (rel_x, rel_y),
                'abs': (abs_x, abs_y),
                'net': net
            }
        
        headers[ref] = {
            'pos': (fp_x, fp_y, fp_rot),
            'pads': pads_dict
        }

    return headers

headers = parse_header_pads('hardware/carrier_shield/bms_carrier_shield.kicad_pcb')
for ref in ['J1', 'J2']:
    h = headers.get(ref)
    if not h:
        print(f"{ref} not found!")
        continue
    print(f"\n=======================================================")
    print(f"HEADER {ref} at {h['pos']}")
    print(f"=======================================================")
    for p in range(1, 39):
        info = h['pads'].get(p)
        if info:
            print(f"Pin {p:2d} | rel: ({info['rel'][0]:6.2f}, {info['rel'][1]:6.2f}) | abs: ({info['abs'][0]:7.2f}, {info['abs'][1]:7.2f}) | Net: {info['net']}")
        else:
            print(f"Pin {p:2d} | NOT FOUND")
