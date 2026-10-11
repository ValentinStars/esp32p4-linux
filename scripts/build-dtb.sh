#!/usr/bin/env bash
# Скрипт компиляции Device Tree для ESP32-P4 Linux
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
KERNEL_DIR="$REPO_ROOT/kernel"

VARIANT="${1:-sv32}"

if [ "$VARIANT" = "nommu" ]; then
    DTS_SRC="$KERNEL_DIR/uc-nommu.dts"
    echo "Компиляция Device Tree для режима NOMMU: $DTS_SRC"
else
    DTS_SRC="$KERNEL_DIR/uc-sv32.dts"
    echo "Компиляция Device Tree для режима Sv32 MMU: $DTS_SRC"
fi

DTB_OUT="$KERNEL_DIR/dtb.bin"

if ! command -v dtc &> /dev/null; then
    echo "Ошибка: утилита 'dtc' (device-tree-compiler) не найдена в PATH!"
    exit 1
fi

dtc -I dts -O dtb -o "$DTB_OUT" "$DTS_SRC"

echo "✓ Дерево устройств успешно скомпилировано в $DTB_OUT"
ls -lh "$DTB_OUT"
