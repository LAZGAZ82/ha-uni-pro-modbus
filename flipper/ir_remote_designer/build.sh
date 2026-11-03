#!/usr/bin/env bash
# Baut die App als .fap -> dist/ir_remote_designer.fap
#
# Variante A (empfohlen, braucht Internet): ufbt
#   pip install ufbt
#   ufbt update            # laedt SDK der aktuellen Release-Firmware
#   ./build.sh             # = ufbt, Ergebnis in dist/
#   ufbt launch            # optional: auf den angeschlossenen Flipper kopieren und starten
#
# Variante B (ohne Zugriff auf update.flipperzero.one): Firmware-Quellen + System-Toolchain
#   git clone --depth 1 -b 1.5.1 https://github.com/flipperdevices/flipperzero-firmware
#   cd flipperzero-firmware && git submodule update --init --depth 1
#   ln -s /pfad/zu/ir_remote_designer applications_user/ir_remote_designer
#   FBT_NOENV=1 ./fbt fap_ir_remote_designer
#   -> build/f7-firmware-D/.extapps/ir_remote_designer.fap
set -euo pipefail
cd "$(dirname "$0")"
if ! command -v ufbt >/dev/null 2>&1; then
    echo "ufbt nicht gefunden: pip install ufbt && ufbt update" >&2
    exit 1
fi
ufbt
echo "Fertig: $(pwd)/dist/ir_remote_designer.fap"
