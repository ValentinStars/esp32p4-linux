#!/bin/sh
# test_interactive.sh - Проверка ANSI последовательностей, цветов и псевдографики
echo "=== ТЕСТ ТЕРМИНАЛА И ANSI-ПОСЛЕДОВАТЕЛЬНОСТЕЙ ==="
echo "Терминал (TERM): $TERM"

echo ""
echo "Таблица цветов ANSI 16/256:"
printf "Базовые цвета: "
for code in 30 31 32 33 34 35 36 37; do
    printf "\033[%sm[%s]\033[0m " "$code" "$code"
done
echo ""

printf "Яркие цвета:   "
for code in 90 91 92 93 94 95 96 97; do
    printf "\033[%sm[%s]\033[0m " "$code" "$code"
done
echo ""

echo ""
echo "Проверка псевдографики UTF-8 / ASCII:"
echo "┌──────────────────────────────────────────────┐"
echo "│  Waveshare ESP32-P4-Module-DEV-KIT Terminal  │"
echo "│  Скорость: 115200 8N1  |  Консоль: ttyS0     │"
echo "└──────────────────────────────────────────────┘"

echo "[  OK  ] Тест терминала успешно пройден!"
