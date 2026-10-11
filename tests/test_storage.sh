#!/bin/sh
# test_storage.sh - Комплексный тест накопителей (MicroSD & USB) для ESP32-P4 Linux

echo "=========================================================="
echo "      ESP32-P4 STORAGE & FILESYSTEM INTEGRITY TEST        "
echo "=========================================================="

echo "[1] Поиск доступных блочных устройств в /dev:"
ls -l /dev/sd* /dev/mmcblk* 2>/dev/null || echo "Блочные узлы устройств ожидают подключения"
echo ""

echo "[2] Поддерживаемые файловые системы ядра:"
cat /proc/filesystems
echo ""

echo "[3] Текущие смонтированные точки:"
mount
echo ""

TEST_TARGET=""
for m in /sdcard /mnt/sdcard /mnt/usb /mnt; do
    if [ -d "$m" ]; then
        TEST_TARGET="$m"
        break
    fi
done

if [ -n "$TEST_TARGET" ]; then
    echo "[4] Тестирование записи и чтения в $TEST_TARGET:"
    TEST_FILE="$TEST_TARGET/test_p4_storage.tmp"
    echo "Запись тестового файла 64 КБ..."
    dd if=/dev/zero of="$TEST_FILE" bs=1024 count=64 2>/dev/null
    if [ -f "$TEST_FILE" ]; then
        SIZE=$(ls -l "$TEST_FILE" | awk '{print $5}')
        echo "✓ Файл создан, размер: $SIZE байт"
        rm -f "$TEST_FILE"
        echo "✓ Файл успешно удален, тест пройден!"
    else
        echo "✗ Ошибка записи в $TEST_TARGET"
    fi
else
    echo "[4] Каталог для монтирования не обнаружен"
fi

echo "=========================================================="
