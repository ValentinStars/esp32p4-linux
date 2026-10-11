#!/usr/bin/env bash
# flash.sh - Скрипт полной прошивки всех компонентов в Waveshare ESP32-P4
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

PORT="${1:-/dev/ttyUSB0}"
BAUD="${2:-921600}"

BOOTLOADER_BIN="$REPO_ROOT/bootloader/build/bootloader/bootloader.bin"
PARTITION_BIN="$REPO_ROOT/bootloader/build/partition_table/partition-table.bin"
APP_BIN="$REPO_ROOT/bootloader/build/esp32p4-linux-boot.bin"
KERNEL_BIN="$REPO_ROOT/kernel/Image"
DTB_BIN="$REPO_ROOT/kernel/dtb.bin"

echo "=========================================================="
echo "  Прошивка ESP32-P4 Linux на плате Waveshare             "
echo "=========================================================="
echo "Порт:     $PORT"
echo "Скорость: $BAUD"
echo "Чип:      ESP32-P4"
echo "=========================================================="

# Проверка наличия esptool.py
ESPTOOL=$(command -v esptool.py || command -v esptool || true)
if [ -z "$ESPTOOL" ]; then
    echo "Ошибка: esptool.py не найден в PATH!"
    exit 1
fi

# Проверка файлов ядра и DTB
if [ ! -f "$KERNEL_BIN" ]; then
    echo "Ошибка: Файл ядра $KERNEL_BIN не найден!"
    exit 1
fi

if [ ! -f "$DTB_BIN" ]; then
    echo "Компиляция DTB..."
    "$SCRIPT_DIR/build-dtb.sh" sv32
fi

FLASH_ARGS=(
    --chip esp32p4
    --port "$PORT"
    --baud "$BAUD"
    --before default_reset
    --after hard_reset
    write_flash -z
)

if [ -f "$BOOTLOADER_BIN" ] && [ -f "$PARTITION_BIN" ] && [ -f "$APP_BIN" ]; then
    echo "Прошивка полного комплекта (Загрузчик + Таблица + Гипервизор + Ядро + DTB)..."
    FLASH_ARGS+=(
        0x0000   "$BOOTLOADER_BIN"
        0x2000   "$PARTITION_BIN"
        0x10000  "$APP_BIN"
        0x110000 "$KERNEL_BIN"
        0xF10000 "$DTB_BIN"
    )
else
    echo "[Внимание] Скомпилированные бинарники загрузчика не найдены в bootloader/build/."
    echo "Прошивка только разделов Kernel (0x110000) и DTB (0xF10000)..."
    FLASH_ARGS+=(
        0x110000 "$KERNEL_BIN"
        0xF10000 "$DTB_BIN"
    )
fi

echo "Выполнение команды:"
echo "$ESPTOOL" "${FLASH_ARGS[@]}"

"$ESPTOOL" "${FLASH_ARGS[@]}"

echo "✓ Прошивка успешно завершена!"
echo "Для подключения к консоли выполните: ./scripts/monitor.sh $PORT"
