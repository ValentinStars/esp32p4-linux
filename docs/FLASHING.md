# Инструкция по прошивке и подключению к терминалу

## 1. Аппаратное подключение платы Waveshare ESP32-P4-Module-DEV-KIT

Отладочная плата оснащена двумя разъемами USB Type-C:
1. **Разъем USB-UART (CH343/CP2102):**
   - Подключен к аппаратным выводам UART0 чипа ESP32-P4:
     - `TXD`: **GPIO 8**
     - `RXD`: **GPIO 7**
   - Используется для прошивки через загрузчик ROM и основной последовательной консоли.
2. **Разъем ESP USB (Native USB 2.0 / USB Serial JTAG):**
   - Подключен напрямую к внутреннему аппаратному блоку USB Serial JTAG чипа ESP32-P4 (GPIO 19 / GPIO 20).
   - Гипервизор опрашивает оба порта одновременно: ввод и вывод работают синхронно через любой из подключенных разъемов!

---

## 2. Параметры последовательного порта (UART)

- **Скорость (Baudrate):** `115200`
- **Биты данных (Data bits):** `8`
- **Четность (Parity):** `None (N)`
- **Стоп-биты (Stop bits):** `1`
- **Управление потоком (Flow Control):** `None`
- **Эмуляция терминала:** `vt100` или `xterm-256color`

---

## 3. Схема прошивки Flash-памяти (16 МБ)

При прошивке через `esptool.py` все компоненты записываются по фиксированным адресам таблицы разделов:

| Компонент | Файл | Смещение Flash |
|---|---|---|
| **Bootloader (ESP-IDF)** | `bootloader/build/bootloader/bootloader.bin` | `0x000000` |
| **Partition Table** | `bootloader/build/partition_table/partition-table.bin` | `0x002000` |
| **Hypervisor Application** | `bootloader/build/esp32p4-linux-boot.bin` | `0x010000` |
| **Linux Kernel Image (с Rootfs)** | `kernel/Image` | `0x110000` |
| **Device Tree (DTB)** | `kernel/dtb.bin` | `0xF10000` |

### Единая команда прошивки на скорости 921600 бод:
```bash
esptool.py --chip esp32p4 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z \
    0x0000   bootloader/build/bootloader/bootloader.bin \
    0x2000   bootloader/build/partition_table/partition-table.bin \
    0x10000  bootloader/build/esp32p4-linux-boot.bin \
    0x110000 kernel/Image \
    0xF10000 kernel/dtb.bin
```

---

## 4. Скрипты автоматизации

Для удобства подготовлены скрипты:

```bash
# Автоматическая прошивка всех компонентов:
./scripts/flash.sh /dev/ttyUSB0

# Прошивка только ядра Linux (для быстрой итерации разработки):
./scripts/flash-kernel.sh /dev/ttyUSB0

# Подключение к консоли Linux:
./scripts/monitor.sh /dev/ttyUSB0
```

---

## 5. Проверка после загрузки и тестовые команды

После включения питания плата выводит лог инициализации ESP-IDF, запускает гипервизор и начинает загрузку Linux. Через ~15 секунд в терминале появится приглашение командной строки:

```
[    0.000000] Linux version 6.9.0-rc1+ ...
...
[  OK  ] Mounted /proc, /sys, /dev, /tmp
[  OK  ] Initialized network stack and loopback interface
Starting login shell on ttyS0...

Welcome to ESP32-P4 RISC-V Linux!
esp32p4-linux login: root
```

### Команды для первичной верификации ОС:
```bash
# 1. Информация о ядре и архитектуре
uname -a

# 2. Проверка памяти и кэша
free -m
cat /proc/meminfo

# 3. Проверка процессора и bogomips
cat /proc/cpuinfo

# 4. Проверка дерева процессов
ps aux
pstree

# 5. Тестирование работы fork и памяти
/usr/bin/memstress

# 6. Запуск консольных игр
cowsay "Linux on ESP32-P4 RISC-V works!"
fortune
tetris
```
