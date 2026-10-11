#!/bin/sh
# test_network.sh - Тестирование сетевого стека, интерфейсов и загрузки по HTTP
set -e

echo "=== ТЕСТ СЕТИ: Проверка сетевого стека ESP32-P4 ==="

echo "[1] Список сетевых интерфейсов:"
ifconfig -a 2>/dev/null || ip addr show 2>/dev/null || true

echo "[2] Проверка локального интерфейса (Loopback ping):"
if ping -c 2 127.0.0.1 > /dev/null 2>&1; then
    echo "[  OK  ] Пинг loopback (127.0.0.1) успешен!"
else
    echo "[ FAIL ] Ошибка пинга loopback!"
fi

echo "[3] Проверка маршрутизации и шлюза:"
route -n 2>/dev/null || ip route show 2>/dev/null || true

echo "[4] Тест DNS резолвера и внешнего соединения:"
GW_IP=$(route -n 2>/dev/null | grep '^0.0.0.0' | awk '{print $2}' || true)
if [ -n "$GW_IP" ]; then
    echo "Попытка пинга шлюза $GW_IP..."
    ping -c 2 "$GW_IP" 2>/dev/null || echo "Шлюз не ответил на ICMP (возможно заблокирован)"
else
    echo "Внешний шлюз по умолчанию не задан. Для подключения настройте sl0 или eth0."
fi

echo "[5] Проверка HTTP/HTTPS клиентов (wget / curl):"
if command -v curl >/dev/null 2>&1; then
    echo "Доступен curl. Проверка запроса заголовков..."
    curl -I -s --connect-timeout 5 http://example.com 2>/dev/null | head -n 3 || echo "Требуется активное внешнее соединение."
elif command -v wget >/dev/null 2>&1; then
    echo "Доступен wget. Проверка тестового запроса..."
    wget -q -O - -T 5 http://example.com 2>/dev/null | head -n 3 || echo "Требуется активное внешнее соединение."
else
    echo "curl/wget не найдены."
fi

echo "=== Тестирование сетевой подсистемы завершено ==="
