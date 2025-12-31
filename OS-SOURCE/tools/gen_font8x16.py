#!/usr/bin/env python3
import re
from pathlib import Path

src = Path('exemplos/Bitmap Text/src/bitmap.c')
out = Path('source/include/font8x16.h')
if not src.exists():
    print('bitmap.c not found')
    raise SystemExit(1)
text = src.read_text()
# find ASCII_BITMAPS array
m = re.search(r'uint32\s+ASCII_BITMAPS\s*\[\s*127\s*\]\s*\[\s*BITMAP_SIZE\s*\]\s*=\s*\{(.*?)\};', text, re.S)
if not m:
    print('ASCII_BITMAPS not found')
    raise SystemExit(1)
content = m.group(1)
# split entries by '},' at top level
entries = re.findall(r'\{(.*?)\}', content, re.S)
# entries correspond to characters starting at 0
# each entry has 16 numbers (binary form like 0b... or hex)
font = []
for ent in entries:
    nums = re.findall(r'0b[01]+|0x[0-9A-Fa-f]+|\d+', ent)
    row16 = [int(n,0) for n in nums]
    # ensure 16 rows
    if len(row16) < 16:
        # pad
        row16 += [0]*(16-len(row16))
    # convert each 16-bit row to 8-bit by taking center bits 4..11
    row8 = []
    for r in row16:
        r = r & 0xFFFF
        # take bits 4..11 (counting LSB=0)
        val = (r >> 4) & 0xFF
        row8.append(val)
    font.append(row8)
# Ensure 128 entries
while len(font) < 128:
    font.append([0]*16)

# write header
out.parent.mkdir(parents=True, exist_ok=True)
with out.open('w') as f:
    f.write('#pragma once\n')
    f.write('#include <stdint.h>\n\n')
    f.write('static const uint8_t font8x16[128][16] = {\n')
    for i, rows in enumerate(font):
        f.write('    {')
        f.write(','.join('0x%02X' % b for b in rows))
        f.write('}')
        if i != len(font)-1:
            f.write(',\n')
        else:
            f.write('\n')
    f.write('};\n')
print('Wrote', out)
