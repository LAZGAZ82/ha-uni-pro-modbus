#!/usr/bin/env bash
# Host-Rendertest: baut die View mit Shim und erzeugt PNGs in out/
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p out
gcc -std=gnu11 -O0 -g -Wall -Ishim -I../.. \
    test_main.c shim.c ../../views/ird_remote_view.c ../../ird_icons.c ../../ird_templates.c \
    -o out/hosttest
./out/hosttest
python3 - <<'PY'
from PIL import Image
import glob, os
for f in sorted(glob.glob('out/*.pbm')):
    im = Image.open(f).convert('L')
    im = im.resize((128*4, 64*4), Image.NEAREST)
    im.save(f[:-4] + '.png')
    os.remove(f)
print("PNGs in out/")
PY
