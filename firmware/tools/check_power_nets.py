import re

with open('hardware/carrier_shield/bms_carrier_shield.kicad_pcb', 'r', encoding='utf-8') as f:
    text = f.read()

print("Pads with E5V:")
for line in text.split('\n'):
    if 'E5V' in line:
        print(line)

print("\nPads with +5V:")
five_v_lines = [l for l in text.split('\n') if '(net "+5V")' in l or '(net 1 "+5V")' in l]
print(f"Total pads on +5V: {len(five_v_lines)}")
