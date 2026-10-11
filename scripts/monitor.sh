#!/usr/bin/env bash
# monitor.sh - Подключение к терминальной консоли ESP32-P4 Linux
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"

echo "Подключение к $PORT на скорости $BAUD..."

if [ -f "$SCRIPT_DIR/term.py" ]; then
    python3 "$SCRIPT_DIR/term.py" "$PORT" "$BAUD"
elif command -v picocom >/dev/null 2>&1; then
    picocom -b "$BAUD" --imap lfcrlf "$PORT"
elif command -v minicom >/dev/null 2>&1; then
    minicom -D "$PORT" -b "$BAUD"
elif command -v screen >/dev/null 2>&1; then
    screen "$PORT" "$BAUD"
elif command -v python3 >/dev/null 2>&1; then
    python3 -m serial.tools.miniterm "$PORT" "$BAUD"
else
    echo "Ошибка: не найден ни один терминальный клиент (term.py, picocom, minicom, screen, miniterm)!"
    exit 1
fi
