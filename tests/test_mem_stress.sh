#!/bin/sh
# test_mem_stress.sh - Стресс-тест оперативной памяти и процессов (fork/vfork)
set -e

echo "=== ТЕСТ 1: Проверка доступной памяти ==="
TOTAL_MEM=$(grep MemTotal /proc/meminfo | awk '{print $2}')
FREE_MEM=$(grep MemFree /proc/meminfo | awk '{print $2}')
echo "Всего памяти: ${TOTAL_MEM} kB, Свободно: ${FREE_MEM} kB"

if [ "$FREE_MEM" -lt 1024 ]; then
    echo "[ВНИМАНИЕ] Свободно менее 1 МБ памяти!"
fi

echo "=== ТЕСТ 2: Стресс-тест выделения блоков памяти через tmpfs ==="
TEST_FILE="/tmp/stress_test.bin"
BLOCKS=4 # 4 x 256KB = 1MB

echo "Запись $BLOCKS блоков по 256KB в tmpfs..."
dd if=/dev/zero of="$TEST_FILE" bs=256k count="$BLOCKS" 2>/dev/null
sync

ls -lh "$TEST_FILE"
echo "Контрольное чтение данных..."
dd if="$TEST_FILE" of=/dev/null bs=256k 2>/dev/null
rm -f "$TEST_FILE"
echo "[  OK  ] Тест памяти в tmpfs пройден успешно!"

echo "=== ТЕСТ 3: Проверка параллельного ветвления процессов (fork) ==="
NUM_FORKS=10
echo "Запуск $NUM_FORKS параллельных подпроцессов..."

pids=""
i=1
while [ $i -le $NUM_FORKS ]; do
    (
        sleep 1
        echo "  [Подпроцесс #$i] PID=$$ отработал успешно."
    ) &
    pids="$pids $!"
    i=$((i + 1))
done

echo "Ожидание завершения подпроцессов..."
for pid in $pids; do
    wait "$pid"
done

echo "[  OK  ] Все $NUM_FORKS процессов fork завершились корректно!"
echo "=== Все стресс-тесты памяти и fork успешно пройдены! ==="
