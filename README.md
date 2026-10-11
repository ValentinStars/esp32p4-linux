# ESP32-P4 RISC-V Linux BSP & Distribution

Полнофункциональный легковесный дистрибутив Linux под архитектуру RISC-V для отладочной платы **Waveshare ESP32-P4-Module-DEV-KIT** (ESP32-P4, двухъядерный RISC-V RV32IMAFCP @ 400 МГц, 16/32 МБ PSRAM).

---

## 1. О проекте

Цель данного проекта — создание полноценной встраиваемой операционной системы Linux для двухъядерного RISC-V микроконтроллера ESP32-P4:
- Полноценное дерево инициализации (`busybox init` с `/etc/inittab`, `/etc/init.d/rcS`, монтированием devtmpfs, proc, sys, tmpfs).
- Полнофункциональная командная оболочка **GNU Bash** со скриптами, job control, пайплайнами.
- Корректное управление терминалом TTY на порту UART / USB-Serial-JTAG (VT100 / xterm-256color, ANSI-escape, getty).
- Набор системных утилит: Coreutils, `grep`, `sed`, `awk`, `curl`, `wget`, `htop`, `vi`/`nano`.
- Сетевой стек TCP/IP, поддержка SLIP/PPP туннелирования и Ethernet.
- Набор консольных игр и бенчмарков (`cowsay`, `fortune`, `tetris`, stress-ng, pthread/fork тесты).

---

## 2. Архитектура системы

Подробное инженерное обоснование приведено в [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

- **Первая ступень (Bootloader / Hypervisor Layer):**
  Прошивка на базе ESP-IDF v5.3+ конфигурирует тактирование чипа (400 МГц), инициализирует внешнюю память PSRAM (200 МГц Hexa-SPI), кэш L1/L2 и реализует виртуализированную RISC-V платформу с трансляцией страниц **Sv32 MMU**, таймером **CLINT**, контроллером прерываний **PLIC** и интерфейсом **SBI 0.2**. Для максимальной производительности используется технология **NCE (Native Code Execution)** и block JIT.
- **Ядро ОС (Linux Kernel):**
  Апстрим Linux 6.6+ (`ARCH=riscv`, `CONFIG_32BIT=y`, `CONFIG_MMU=y`, `CONFIG_SERIAL_8250=y`).
- **Корневая файловая система (Rootfs):**
  Сборка на базе Buildroot с библиотекой `musl`/`uclibc-ng`, встроенным initramfs, полным набором утилит Unix и шеллом GNU Bash.

---

## 3. Структура репозитория

```
├── bootloader/              # Исходный код первой ступени / гипервизора (ESP-IDF)
│   ├── CMakeLists.txt       # Конфигурация сборки ESP-IDF
│   ├── partitions.csv       # Таблица разделов 16 МБ Flash
│   ├── sdkconfig.defaults   # Параметры оптимизации PSRAM 200МГц и кэша
│   └── main/                # Ядро гипервизора (mini-rv32ima, NCE JIT, UART, SBI)
├── kernel/                  # Конфигурации и Device Tree для ядра Linux
│   ├── uc-sv32.dts          # Дерево устройств для MMU Sv32 режима
│   ├── uc-nommu.dts         # Дерево устройств для NOMMU режима
│   └── defconfig            # Оптимизированный конфиг ядра Linux
├── rootfs/                  # Настройки файловой системы и оверлей
│   ├── etc/                 # inittab, fstab, profile, init.d/rcS, network
│   ├── bin/                 # Бинарные утилиты и сценарии
│   └── usr/                 # Системные данные, terminfo, игры
├── docs/                    # Полная инженерная документация
│   ├── ARCHITECTURE.md      # Архитектурный анализ MMU vs NOMMU
│   ├── MEMORY_MAP.md        # Карта Flash и PSRAM памяти
│   ├── BUILD.md             # Пошаговая инструкция по сборке
│   ├── FLASHING.md          # Инструкция по прошивке и подключению
│   ├── PERIPHERALS.md       # Интеграция SDMMC, MIPI, USB OTG, Ethernet
│   └── TROUBLESHOOTING.md   # Руководство по устранению неполадок
├── scripts/                 # Автоматизация сборки и прошивки
│   ├── build-dtb.sh         # Компиляция DTS в DTB
│   ├── build-rootfs.sh      # Сборка cpio initramfs
│   ├── flash.sh             # Автоматическая прошивка через esptool.py
│   └── monitor.sh           # Подключение к консоли UART
├── tests/                   # Набор тестов и бенчмарков
│   ├── test_mem_stress.sh   # Стресс-тест памяти и fork
│   ├── test_network.sh      # Проверка TCP/IP стека и curl
│   └── test_interactive.sh  # Проверка терминала и псевдографики
└── tools/                   # Кросс-компиляторы и утилиты хоста
```

---

## 4. Быстрый старт: сборка и прошивка

### 4.1 Компиляция Device Tree
```bash
./scripts/build-dtb.sh
```

### 4.2 Сборка загрузчика / гипервизора (ESP-IDF)
```bash
cd bootloader
idf.py set-target esp32p4
idf.py build
cd ..
```

### 4.3 Прошивка в плату
Подключите Waveshare ESP32-P4 через USB-UART / Type-C порт:
```bash
./scripts/flash.sh /dev/ttyUSB0
```

Либо вручную через `esptool.py`:
```bash
esptool.py --chip esp32p4 -b 921600 write_flash \
    0x0000 bootloader/build/bootloader/bootloader.bin \
    0x2000 bootloader/build/partition_table/partition-table.bin \
    0x10000 bootloader/build/esp32p4-linux-boot.bin \
    0x110000 kernel/Image \
    0xF10000 kernel/dtb.bin
```

### 4.4 Подключение к терминалу
```bash
picocom -b 115200 /dev/ttyUSB0
```
Скорость консоли: **115200 бод**, 8N1. Также поддерживается прямой USB Serial/JTAG встроенного порта ESP32-P4.
