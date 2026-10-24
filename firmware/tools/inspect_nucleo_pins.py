import re

def analyze_sexp():
    with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
        lines = f.readlines()

    current_fp = None
    fp_ref = None
    fp_at = None
    fp_pads = {} # pad_num: (x, y, net)
    
    j_fps = {}

    for line in lines:
        if line.strip().startswith('(footprint '):
            current_fp = line.strip()
            fp_ref = None
            fp_at = None
            fp_pads = {}
        elif current_fp is not None:
            if '(property "Reference"' in line:
                m = re.search(r'property "Reference"\s+"([^"]+)"', line)
                if m:
                    fp_ref = m.group(1)
            elif line.strip().startswith('(at ') and fp_at is None:
                # footprint position
                m = re.search(r'\(at\s+([0-9.-]+)\s+([0-9.-]+)(?:\s+([0-9.-]+))?\)', line)
                if m:
                    fp_at = (float(m.group(1)), float(m.group(2)), float(m.group(3)) if m.group(3) else 0.0)
            elif '(pad ' in line:
                m_pad = re.search(r'\(pad\s+"?(\d+)"?\s+\S+\s+\S+\s+\(at\s+([0-9.-]+)\s+([0-9.-]+)(?:\s+([0-9.-]+))?\)(?:.*?\(net\s+"?([^"]+)"?\))?', line)
                # Pad might be multi-line
                pass
            
            if line.strip() == ')' and fp_ref in ['J1', 'J2', 'H1', 'H2', 'H3', 'H4']:
                j_fps[fp_ref] = (current_fp, fp_at)
                current_fp = None

    print("Found special footprints in PCB:")
    for k, v in j_fps.items():
        print(f"  {k}: at {v[1]} | {v[0][:60]}")

analyze_sexp()
