#!/bin/bash
# build_official_network.sh - Автоматическая кросс-компиляция официальных репозиториев nmap, ncat и netcat под RISC-V RV32
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLCHAIN_BIN="$REPO_ROOT/tools/riscv32-toolchain/bin"
CC="$TOOLCHAIN_BIN/riscv32-buildroot-linux-musl-gcc"
CXX="$TOOLCHAIN_BIN/riscv32-buildroot-linux-musl-g++"
STRIP="$TOOLCHAIN_BIN/riscv32-buildroot-linux-musl-strip"
OUT_DIR="$REPO_ROOT/tools/bin"

mkdir -p "$OUT_DIR" "$REPO_ROOT/third_party"
cd "$REPO_ROOT/third_party"

# 1. Официальный Netcat Traditional (The Hobbit 1.10)
echo "=== 1. Сборка официального Netcat Traditional ==="
if [ ! -d "netcat-traditional" ]; then
    git clone --depth 1 https://salsa.debian.org/debian/netcat.git netcat-traditional
fi
(
    cd netcat-traditional
    make clean || true
    make CC="$CC" linux
    "$STRIP" nc
    cp nc "$OUT_DIR/nc"
)

# 2. Официальный Nmap & Ncat
echo "=== 2. Сборка официального Nmap & Ncat ==="
if [ ! -d "nmap" ]; then
    git clone --depth 1 https://github.com/nmap/nmap.git nmap
fi
(
    cd nmap
    CC="$CC" CXX="$CXX" CFLAGS="-O2 -static" CXXFLAGS="-O2 -static" LDFLAGS="-static" \
    ./configure \
      --host=riscv32-linux-musl \
      --without-zenmap \
      --without-ndiff \
      --without-nping \
      --without-openssl \
      --without-libssh2 \
      --without-liblua \
      --with-libpcap=included \
      --with-libpcre=included \
      --with-libdnet=included \
      --with-liblinear=included \
      --with-libz=included
    make -j"$(nproc)"
    "$STRIP" nmap ncat/ncat
    cp ncat/ncat "$OUT_DIR/ncat"
    cp nmap "$OUT_DIR/nmap"
)

echo "✓ Все официальные сетевые утилиты успешно собраны в $OUT_DIR:"
ls -lh "$OUT_DIR"
