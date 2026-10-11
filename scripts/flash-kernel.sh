#!/usr/bin/env bash
# flash-kernel.sh - Быстрая прошивка только ядра Linux и DTB (без перезаписи загрузчика)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

PORT="${1:-/dev/ttyUSB0}"
BAUD="${2:-921600}"

KERNEL_BIN="$REPO_ROOT/kernel/Image"
DTB_BIN="$REPO_ROOT/kernel/dtb.bin"

ESPTOOL=$(command -v esptool.py || command -v esptool || true)
if [ -z "$ESPTOOL" ]; then
    echo "Ошибка: esptool.py не найден в PATH!"
    exit 1
fi

if [ ! -f "$DTB_BIN" ]; then
    "$SCRIPT_DIR/build-dtb.sh" sv32
fi

echo "Быстрая прошивка Kernel (0x110000) и DTB (0xF10000) на $PORT ($BAUD)..."

"$ESPTOOL" --chip esp32p4 --port "$PORT" --baud "$BAUD" \
    --before default_reset --after hard_reset write_flash -z \
    0x110000 "$KERNEL_BIN" \
    0xF10000 "$DTB_BIN"

echo "✓ Ядро успешно обновлено!"
