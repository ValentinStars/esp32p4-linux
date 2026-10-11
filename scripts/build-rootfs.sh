#!/usr/bin/env bash
# build-rootfs.sh - Упаковка файловой системы в CPIO initramfs архив
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
ROOTFS_DIR="$REPO_ROOT/rootfs"
OUT_CPIO="$REPO_ROOT/kernel/rootfs.cpio"
OUT_CPIO_GZ="$REPO_ROOT/kernel/rootfs.cpio.gz"

echo "=== Сборка Initramfs Rootfs для ESP32-P4 ==="
echo "Исходный каталог: $ROOTFS_DIR"

# 1. Проверка прав исполняемых файлов
chmod +x "$ROOTFS_DIR"/init
chmod +x "$ROOTFS_DIR"/etc/init.d/*
chmod +x "$ROOTFS_DIR"/bin/*
chmod +x "$ROOTFS_DIR"/usr/bin/*

# 2. Создание необходимых базовых узлов устройств в /dev если их нет
mkdir -p "$ROOTFS_DIR/dev"
if [ ! -e "$ROOTFS_DIR/dev/console" ]; then
    mknod -m 600 "$ROOTFS_DIR/dev/console" c 5 1 2>/dev/null || true
fi
if [ ! -e "$ROOTFS_DIR/dev/null" ]; then
    mknod -m 666 "$ROOTFS_DIR/dev/null" c 1 3 2>/dev/null || true
fi
if [ ! -e "$ROOTFS_DIR/dev/zero" ]; then
    mknod -m 666 "$ROOTFS_DIR/dev/zero" c 1 5 2>/dev/null || true
fi

# 3. Упаковка в CPIO архив формата newc
echo "Упаковка файлов в CPIO..."
(cd "$ROOTFS_DIR" && find . -print0 | cpio --null --create --format=newc --owner=root:root) > "$OUT_CPIO"

# 4. Создание сжатой версии GZIP
echo "Сжатие архива с помощью gzip..."
gzip -9 -c "$OUT_CPIO" > "$OUT_CPIO_GZ"

echo "✓ Initramfs успешно собран:"
ls -lh "$OUT_CPIO" "$OUT_CPIO_GZ"
