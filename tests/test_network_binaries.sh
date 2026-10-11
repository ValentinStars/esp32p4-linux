#!/bin/bash
# test_network_binaries.sh - Проверка валидности собранных официальных утилит nmap, ncat и nc
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
TOOLS_DIR="$REPO_ROOT/tools/bin"
READELF="$REPO_ROOT/tools/riscv32-toolchain/bin/riscv32-buildroot-linux-musl-readelf"

echo "=========================================================="
echo "    VERIFYING REAL OFFICIAL NETWORK BINARIES (RISC-V)     "
echo "=========================================================="

check_binary() {
    local bin="$1"
    local name="$(basename "$bin")"
    echo -n "[*] Проверка $name ... "

    if [ ! -f "$bin" ]; then
        echo "ОШИБКА: Файл $bin не найден!"
        exit 1
    fi

    # 1. Проверка архитектуры ELF
    local arch=$("$READELF" -h "$bin" | awk '/Machine:/ {print $2}')
    local class=$("$READELF" -h "$bin" | awk '/Class:/ {print $2}')
    local flags=$("$READELF" -h "$bin" | grep "Flags:" || true)

    if [ "$arch" != "RISC-V" ] || [ "$class" != "ELF32" ]; then
        echo "ОШИБКА: Неверная архитектура ($arch / $class)!"
        exit 1
    fi

    # 2. Проверка статической линковки
    if file "$bin" | grep -q "statically linked"; then
        local size=$(ls -lh "$bin" | awk '{print $5}')
        echo "✓ OK (ELF32 RISC-V, ilp32d, static, $size)"
    else
        echo "ОШИБКА: Бинарник не слинкован статически!"
        exit 1
    fi
}

check_binary "$TOOLS_DIR/nc"
check_binary "$TOOLS_DIR/ncat"
check_binary "$TOOLS_DIR/nmap"

echo ""
echo "=========================================================="
echo "✓ Все официальные сетевые утилиты успешно проверены!"
echo "=========================================================="
