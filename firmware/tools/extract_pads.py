import re

def extract_pads():
    with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
        text = f.read()

    # Split footprints
    fps = text.split('(footprint ')
    for fp in fps[1:]:
        # check reference
        m_ref = re.search(r'property "Reference"\s+"([^"]+)"', fp)
        if not m_ref:
            continue
        ref = m_ref.group(1)
        if ref not in ['J1', 'J2']:
            continue
        
        # footprint position
        m_at = re.search(r'\(at\s+([0-9.-]+)\s+([0-9.-]+)(?:\s+([0-9.-]+))?\)', fp)
        fp_x = float(m_at.group(1))
        fp_y = float(m_at.group(2))
        fp_rot = float(m_at.group(3)) if m_at.group(3) else 0.0
        
        print(f"\n==========================================")
        print(f"FOOTPRINT {ref}: at ({fp_x}, {fp_y}, rot={fp_rot})")
        print(f"==========================================")

        # extract pads: (pad "1" thru_hole ... (at 0 0) ... (net 12 "NetName"))
        # or pad lines
        pad_matches = re.finditer(r'\(pad\s+"(\d+)"\s+(\S+)\s+(\S+)\s+\(at\s+([0-9.-]+)\s+([0-9.-]+)(?:\s+([0-9.-]+))?\).*?\(net\s+\d+\s+"([^"]*)"\)', fp, re.DOTALL)
        pads = []
        for pm in pad_matches:
            p_num = int(pm.group(1))
            p_type = pm.group(2)
            p_shape = pm.group(3)
            p_rel_x = float(pm.group(4))
            p_rel_y = float(pm.group(5))
            p_net = pm.group(7)
            abs_x = fp_x + p_rel_x
            abs_y = fp_y + p_rel_y
            pads.append((p_num, abs_x, abs_y, p_rel_x, p_rel_y, p_net))

        pads.sort(key=lambda x: x[0])
        for p in pads:
            print(f"  Pin {p[0]:2d}: abs=({p[1]:7.2f}, {p[2]:7.2f}) rel=({p[3]:6.2f}, {p[4]:6.2f}) Net={p[5]}")

extract_pads()
