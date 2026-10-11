#!/usr/bin/env bash
# monitor.sh - Подключение к терминалу ESP32-P4 без сброса платы
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"

# Отключаем сброс при закрытии дескриптора на уровне ядра Linux
stty -F "$PORT" -hupcl 2>/dev/null || true

# Запускаем наш POSIX-терминал, который не трогает DTR/RTS
exec python3 "$SCRIPT_DIR/term.py" "$PORT" "$BAUD"
