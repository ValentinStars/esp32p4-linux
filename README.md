# ESP32-P4 RISC-V Linux Cyberdeck & BSP

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arch: RISC-V RV32IMA](https://img.shields.io/badge/Arch-RISC--V%20RV32IMA-blue.svg)](https://riscv.org/)
[![Kernel: Linux 6.9 NOMMU](https://img.shields.io/badge/Kernel-Linux%206.9%20NOMMU-orange.svg)](https://kernel.org/)
[![Hardware: Waveshare ESP32-P4](https://img.shields.io/badge/Hardware-Waveshare%20ESP32--P4--Module--DEV--KIT--C-green.svg)](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT)

Полнофункциональная автономная операционная система **Linux RISC-V** для отладочного комплекса **Waveshare ESP32-P4-Module-DEV-KIT-C** (двухъядерный 32-битный RISC-V процессор @ 400 МГц, 32 МБ аппаратной PSRAM APMemory, 16 МБ Flash, сопроцессор Wi-Fi 6 / Bluetooth 5 ESP32-C6, слот MicroSD TF Card, USB 2.0 High-Speed OTG Host, интерфейс экрана MIPI-DSI).

---

## 🚀 Возможности системы

* **Ядро Linux 6.9.0 NOMMU:** 
  * Настоящее ядро Linux для микроконтроллеров без виртуальной памяти.
  * Время загрузки до шелла: **~1.5 секунды**.
  * Поддержка многозадачности, дерева процессов, сигналов, сокетов, пайпов.
* **Нативная консоль и утилиты:**
  * Интерактивный шелл **BusyBox ash** с поддержкой ANSI цветов.
  * Утилита сбора информации **`fastfetch`** с цветным RISC-V ASCII логотипом и характеристиками платы.
  * Набор утилит Unix: `vi`, `top`, `ps`, `df`, `free`, `uptime`, `dmesg`, `sed`, `grep`, `tar`.
* **Хранилище и накопители:**
  * Полноскоростной слот для карт памяти **MicroSD (SDMMC Slot 1, 4-bit bus @ 40 МГц)**.
  * Поддержка файловых систем **FAT32 (vfat)** и **ext4**.
  * Поддержка внешних флешек через **USB 2.0 Host (MSC Class)**.
* **Графика и Мультимедиа:**
  * Вывод видеокадров и текстовой консоли через интерфейс **MIPI-DSI (2-lane)** на дисплей Dev-Kit.
  * Проигрыватель видео **Bad Apple** с аппаратной кадровой синхронизацией.
* **Автономный кибердек:**
  * Подключение стандартной USB-клавиатуры через порт Type-A в режиме **USB HID Host**.
  * Полностью автономная работа без необходимости подключения к ПК!
* **Беспроводная связь:**
  * Архитектура сетевого моста к чипу **ESP32-C6 (Wi-Fi 6 & Bluetooth 5)** через SDIO шину по протоколу **ESP-Hosted**.

---

## 🛠️ Архитектура оборудования

```
       ┌────────────────────────────────────────────────────────┐
       │             WAVESHARE ESP32-P4 MODULE                  │
       │                                                        │
       │   [ESP32-P4 Dual-Core @ 400MHz] <──SDIO──> [ESP32-C6] │
       │     │ (32MB PSRAM + 16MB Flash)          (Wi-Fi 6/BT)  │
       │     │                                                  │
       │     ├── [MIPI-DSI 2-lane]  ──> Framebuffer Display     │
       │     ├── [SDMMC GPIO 39-44] ──> MicroSD TF Card Slot    │
       │     ├── [USB 2.0 HS PHY]   ──> USB-A Keyboard / Flash  │
       │     └── [Ethernet RMII]    ──> RJ45 100M Port          │
       └────────────────────────────────────────────────────────┘
```

---

## 📁 Структура репозитория

```
├── bootloader/              # Гипервизор / первая ступень на базе ESP-IDF
│   ├── CMakeLists.txt       # Конфигурация сборки с SDMMC и FATFS
│   ├── partitions.csv       # Карта разделов 16 МБ SPI Flash
│   ├── platformio.ini       # Сборка через PlatformIO
│   └── main/                # Исходный код эмулятора RV32IMA, SDMMC, JIT, UART
├── kernel/                  # Ядро Linux 6.9 RISC-V
│   ├── Image                # Скомпилированный бинарный образ ядра с CPIO
│   ├── dtb.bin              # Скомпилированное бинарное дерево устройств
│   ├── uc-nommu.dts         # Исходный Device Tree для NOMMU
│   └── defconfig            # Конфигурация ядра (VFAT, ext4, initramfs)
├── rootfs/                  # Корневая файловая система
│   ├── bin/                 # Утилиты Busybox, fastfetch
│   ├── fastfetch.c          # Исходный код fastfetch на C
│   ├── badapple.c           # Исходный код плеера Bad Apple на C
│   └── etc/init.d/rcS       # Скрипт автозагрузки ОС
├── scripts/                 # Скрипты автоматизации
│   ├── term.py              # Легковесный надежный терминал (не дергает DTR/RTS)
│   ├── monitor.sh           # Запуск терминала
│   ├── flash-kernel.sh      # Быстрая прошивка ядра
│   └── flash.sh             # Полная прошивка системы
├── tools/                   # Инструменты для хост-системы
│   └── encode_badapple.py   # Конвертер видео MP4 в видеопоток для консоли/FB
├── tests/                   # Набор тестов
│   ├── test_storage.sh      # Тест MicroSD и USB накопителей
│   ├── test_mem_stress.sh   # Стресс-тест памяти PSRAM
│   └── test_network.sh      # Тестирование сетевых соединений
└── docs/                    # Полная инженерная документация
```

---

## ⚡ Быстрый старт

### 1. Подключение к терминалу
Для комфортной работы используйте встроенный терминал:
```bash
./scripts/monitor.sh
```
*(Для выхода нажмите `Ctrl + ]` или `Ctrl + Q`)*.

### 2. Запуск системного информера Fastfetch
В приглашении командной строки введите:
```bash
fastfetch
```

### 3. Прошивка ядра
```bash
./scripts/flash-kernel.sh /dev/ttyACM0 921600
```

---

## 📄 Лицензия

Проект распространяется под свободной лицензией **MIT License**. См. файл [LICENSE](LICENSE).

Автор: **Valentin Stars** (2026).
