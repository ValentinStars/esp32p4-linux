# ESP32-P4 RISC-V Linux Cyberdeck

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arch: RISC-V RV32IMA](https://img.shields.io/badge/Arch-RISC--V%20RV32IMA-blue.svg)](https://riscv.org/)
[![Kernel: Linux 6.9 NOMMU](https://img.shields.io/badge/Kernel-Linux%206.9%20NOMMU-orange.svg)](https://kernel.org/)
[![Target: Waveshare ESP32-P4-Module-DEV-KIT](https://img.shields.io/badge/Board-Waveshare%20ESP32--P4--Module--DEV--KIT-green.svg)](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT)

[English](README.md) | [Русская версия](README_RU.md)

Полнофункциональная автономная операционная система **Linux RISC-V NOMMU** для отладочного комплекса **Waveshare ESP32-P4-Module-DEV-KIT** (двухъядерный 32-битный RISC-V процессор @ 400 МГц, 32 МБ PSRAM APMemory, 16 МБ Flash, сопроцессор Wi-Fi 6 / Bluetooth 5 ESP32-C6, слот MicroSD TF Card, USB 2.0 High-Speed OTG Host, интерфейс экрана MIPI-DSI).

---

## ⚡ Ключевые возможности

- **Ядро Linux 6.9.0 NOMMU:** 
  - Настоящее ядро Linux для микроконтроллеров без MMU.
  - Холодный старт до командной строки: **~1.5 секунды**.
  - Полная многозадачность, процессы, сигналы, пайпы, файловые дескрипторы.
- **Вывод на экран MIPI-DSI (1024×600):**
  - Прямой фреймбуфер RGB565 с частотой обновления 60 кадров/сек.
  - Встроенный эмулятор ANSI VT100 терминала (128 колонок × 37 строк, растровый моноширинный шрифт 8×16, плавный скроллинг, цвета ANSI).
- **Подсистема USB 2.0 Host:**
  - **Драйвер клавиатуры HID:** Поддержка беспроводных 2.4 ГГц донглов и проводных USB клавиатур. Сканкоды транслируются напрямую в поток UART ядра (`/dev/ttyS0` / `/dev/console`).
  - **Драйвер накопителей MSC:** Автоматическое определение флешек и монтирование в `/usbdrive`.
- **Накопители и сохранение данных:**
  - Полноскоростной слот для карт памяти **MicroSD (SDMMC Slot 1, 4-битная шина @ 40 МГц)**.
  - Поддержка файловых систем **FAT32 (vfat)** и **ext4**.
- **Пользовательские утилиты:**
  - Нативный **`fastfetch`** с RISC-V ASCII логотипом и выводом системной телеметрии.
  - Плеер видео **Bad Apple** (`/bin/badapple`) с воспроизведением видеопотоков на 24+ FPS.
  - Набор утилит BusyBox: `sh`, `vi`, `top`, `ps`, `df`, `free`, `dmesg`, `grep`, `tar`.

---

## 🛠️ Архитектура оборудования

```
┌────────────────────────────────────────────────────────────────────────┐
│                   WAVESHARE ESP32-P4 MODULE DEV KIT                    │
│                                                                        │
│   ┌───────────────────────────┐         SDIO        ┌────────────────┐ │
│   │ ESP32-P4 Dual-Core RV32   │ ◄─────────────────► │ ESP32-C6 Wi-Fi │ │
│   │ 400 MHz HP + 40 MHz LP    │                     │ Wi-Fi 6 + BT 5 │ │
│   └─────────────┬─────────────┘                     └────────────────┘ │
│                 │                                                      │
│    ┌────────────┼────────────┬─────────────┬────────────┐              │
│    ▼            ▼            ▼             ▼            ▼              │
│ ┌───────┐  ┌─────────┐  ┌─────────┐   ┌─────────┐  ┌─────────┐         │
│ │ 32 MB │  │ MIPI-DSI│  │ SDMMC   │   │ USB 2.0 │  │ 100M    │         │
│ │ PSRAM │  │ 2-lane  │  │ Slot 1  │   │ DWC HS  │  │ RMII    │         │
│ └───────┘  └────┬────┘  └────┬────┘   └────┬────┘  └────┬────┘         │
│                 │            │             │            │              │
│                 ▼            ▼             ▼            ▼              │
│             1024x600      MicroSD       USB Host     Ethernet          │
│             Дисплей       TF Card     (Клава/Флеш)     RJ45            │
└────────────────────────────────────────────────────────────────────────┘
```

### Важный аппаратный переключатель: Джампер питания USB Host
На плате Waveshare DEV-KIT рядом с разъемом USB Type-A расположен **джампер выбора режима USB OTG**:
- **Положение HOST:** Джампер замыкает контакты `5V / HOST`. Включается повышающий преобразователь 5V для питания подключенной флешки и радиомодуля клавиатуры 2.4 ГГц.
- **Положение DEVICE:** Отключает питание 5V (используется только при подключении платы к ПК в качестве периферийного устройства).

---

## 🗂️ Таблица разделов Flash (16 МБ)

| Смещение     | Раздел          | Размер   | Назначение                                  |
|--------------|-----------------|----------|---------------------------------------------|
| `0x000000`   | `bootloader`    | 8 КБ     | Вторичный загрузчик ESP-IDF                 |
| `0x002000`   | `partition-tbl` | 4 КБ     | Таблица разделов                            |
| `0x009000`   | `nvs`           | 24 КБ    | Non-volatile storage                        |
| `0x00F000`   | `phy_init`      | 4 КБ     | Калибровочные данные радиочасти             |
| `0x010000`   | `factory`       | 1024 КБ  | Гипервизор / Драйверы экрана и USB Host     |
| `0x110000`   | `kernel`        | 14 МБ    | Ядро Linux 6.9 + CPIO RootFS                |
| `0xF10000`   | `dtb`           | 64 КБ    | Скомпилированное дерево устройств DTB       |

---

## 🛠️ Пошаговая инструкция по сборке

### Необходимые инструменты
```bash
# Установка зависимостей на хост-системе (Linux)
sudo apt-get update && sudo apt-get install -y \
    python3 python3-pip python3-serial \
    device-tree-compiler cpio build-essential

# Установка PlatformIO Core и esptool
pip3 install --user platformio esptool
```

### 1. Компиляция дерева устройств (DTB)
```bash
./scripts/build-dtb.sh sv32
```
Результат: `kernel/dtb.bin`.

### 2. Сборка корневой файловой системы RootFS
```bash
./scripts/build-rootfs.sh
```
Результат: `kernel/rootfs.cpio.gz`.

### 3. Компиляция гипервизора и драйверов
```bash
pio run -d bootloader
```
Результат: `bootloader/.pio/build/esp32-p4/firmware.bin`.

---

## ⚡ Прошивка платы

Подключите плату через порт **UART / USB-C** (`/dev/ttyACM0`).

### Полная прошивка (все компоненты)
```bash
./scripts/flash.sh /dev/ttyACM0 460800
```

### Быстрая прошивка только ядра
```bash
./scripts/flash-kernel.sh /dev/ttyACM0 460800
```

---

## 💻 Подключение к терминалу

Стандартные терминалы (minicom, screen) дергают линии DTR/RTS, что приводит к перезагрузке платы.

Используйте встроенный POSIX-клиент, который **не вызывает сброса чипа**:
```bash
./scripts/monitor.sh
```
*(Для выхода нажмите `Ctrl + ]` или `Ctrl + Q`).*

В командной строке Linux:
```bash
# Информация о системе
fastfetch

# Просмотр смонтированных накопителей
mount
df -h

# Воспроизведение Bad Apple
badapple /usbdrive/badapple.bin
```

---

## 📜 Лицензия

Проект распространяется под свободной лицензией [MIT License](LICENSE).  
Автор: **Valentin Stars** (2026).
