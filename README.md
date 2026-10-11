# ESP32-P4 RISC-V Linux Cyberdeck

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arch: RISC-V RV32IMA](https://img.shields.io/badge/Arch-RISC--V%20RV32IMA-blue.svg)](https://riscv.org/)
[![Kernel: Linux 6.9 NOMMU](https://img.shields.io/badge/Kernel-Linux%206.9%20NOMMU-orange.svg)](https://kernel.org/)
[![Target: Waveshare ESP32-P4-Module-DEV-KIT](https://img.shields.io/badge/Board-Waveshare%20ESP32--P4--Module--DEV--KIT-green.svg)](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT)

[English](README.md) | [Русская версия](README_RU.md)

A complete standalone **Linux NOMMU** cyberdeck and operating system environment ported to the **Waveshare ESP32-P4-Module-DEV-KIT** (Dual-Core 32-bit RISC-V @ 400 MHz, 32 MB APMemory PSRAM, 16 MB Flash, ESP32-C6 Wi-Fi 6 / Bluetooth 5 co-processor, MIPI-DSI 2-lane display, SDMMC MicroSD slot, and USB 2.0 High-Speed Host).

---

## ⚡ Highlights

- **Linux Kernel 6.9.0 NOMMU:** Full multitasking kernel boots to userspace shell in **~1.5 seconds**.
- **1024×600 MIPI-DSI Display Output:** Direct RGB565 framebuffer driver with integrated ANSI VT100 console terminal (128 columns × 37 rows, 8×16 monospace bitmap font, smooth scrolling, 60 FPS refresh rate).
- **USB 2.0 Host Subsystem:**
  - **HID Keyboard Driver:** Supports wireless 2.4 GHz dongles and wired USB keyboards. Keypresses are directly dispatched into the Linux UART RX stream (`/dev/ttyS0` / `/dev/console`).
  - **MSC Mass Storage Driver:** Automatic FAT32/ext4 detection and automounting at `/usbdrive`.
- **High-Speed Storage & Persistence:**
  - 4-bit SDMMC Slot 1 (`/sdcard`) running at 40 MHz.
  - Persistent read/write filesystem for files, scripts, and logs.
- **Embedded Userspace Utilities:**
  - Compact POSIX **`fastfetch`** with custom RISC-V ASCII logo and runtime system telemetry.
  - Custom **Bad Apple** stream player (`/bin/badapple`) capable of playing video streams from flash drives or SD cards at 24+ FPS.
  - BusyBox suite: `sh`, `vi`, `top`, `ps`, `df`, `free`, `dmesg`, `grep`, `tar`, and networking utilities.
- **Wireless Networking Architecture:** Bridge via SDIO bus to onboard ESP32-C6 for Wi-Fi 6 (802.11ax) and Bluetooth 5 LE connectivity.

---

## 🏗️ Hardware Architecture & Interconnects

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
│             Display       TF Card      (KB / Flash)    RJ45            │
└────────────────────────────────────────────────────────────────────────┘
```

### Critical Hardware Switch: USB Host VBUS Jumper
On the Waveshare DEV-KIT board, locate the **USB OTG Jumper** adjacent to the Type-A USB receptacle:
- **HOST Mode:** Place jumper cap to bridge `5V / HOST`. This enables the onboard step-up converter to feed +5V VBUS power to connected USB flash drives and wireless 2.4 GHz keyboard receivers.
- **DEVICE Mode:** Disconnects onboard 5V booster (used only when connecting ESP32-P4 directly to a PC as a client).

---

## 🗂️ Memory & Flash Partition Layout

The 16 MB onboard NOR Flash is mapped as follows:

| Offset       | Partition       | Size     | Description                              |
|--------------|-----------------|----------|------------------------------------------|
| `0x000000`   | `bootloader`    | 8 KB     | ESP-IDF 2nd-stage bootloader             |
| `0x002000`   | `partition-tbl` | 4 KB     | ESP-IDF Partition Table                  |
| `0x009000`   | `nvs`           | 24 KB    | Non-volatile storage                     |
| `0x00F000`   | `phy_init`      | 4 KB     | Radio calibration data                   |
| `0x010000`   | `factory`       | 1024 KB  | Emulator hypervisor + Display + USB Host |
| `0x110000`   | `kernel`        | 14 MB    | Prebuilt Linux Kernel 6.9 + CPIO RootFS  |
| `0xF10000`   | `dtb`           | 64 KB    | Compiled Flattened Device Tree Blob      |

---

## 🛠️ Step-by-Step Build Guide

### Prerequisites
Install essential host tools on your Linux machine (Ubuntu/Debian/Arch/Fedora):
```bash
# Ubuntu / Debian
sudo apt-get update && sudo apt-get install -y \
    python3 python3-pip python3-serial \
    device-tree-compiler cpio build-essential

# Install PlatformIO Core
pip3 install --user platformio esptool
```

### 1. Build Device Tree Blob (DTB)
Compile the device tree source for the virtual RISC-V NOMMU platform:
```bash
./scripts/build-dtb.sh sv32
```
Output artifact: `kernel/dtb.bin`.

### 2. Prepare RootFS & Initramfs
Bundle and compress the root filesystem into an initramfs CPIO archive:
```bash
./scripts/build-rootfs.sh
```
Output artifact: `kernel/rootfs.cpio.gz`.

### 3. Build Bootloader & Hypervisor
The hypervisor contains the RV32IMA interpreter/JIT, MIPI-DSI video driver, and USB Host stack. Build using PlatformIO:
```bash
pio run -d bootloader
```
Output artifacts:
- `bootloader/.pio/build/esp32-p4/bootloader.bin`
- `bootloader/.pio/build/esp32-p4/partitions.bin`
- `bootloader/.pio/build/esp32-p4/firmware.bin`

---

## ⚡ Flashing the Board

Connect the board via the **UART / USB-C port** (`/dev/ttyACM0`).

### Full Flash (First Time Setup)
Writes bootloader, partition table, hypervisor, kernel, and DTB:
```bash
./scripts/flash.sh /dev/ttyACM0 460800
```

### Quick Kernel Update
Reflashes only the Linux kernel (`0x110000`) and DTB (`0xF10000`):
```bash
./scripts/flash-kernel.sh /dev/ttyACM0 460800
```

---

## 💻 Connecting & Serial Console

Standard serial monitors (minicom, picocom, default pyserial) assert DTR and RTS on open, causing the ESP32 hardware auto-reset circuit to reboot the board continuously.

Use the custom POSIX client to connect cleanly **without resetting the chip**:
```bash
./scripts/monitor.sh
```
*(To exit the session safely, press `Ctrl + ]` or `Ctrl + Q`).*

Inside the Linux shell:
```bash
# System information
fastfetch

# View block devices and mounts
mount
df -h

# Play Bad Apple on display
badapple /usbdrive/badapple.bin
```

---

## 🎮 Standalone Cyberdeck Operation

1. Connect the **Waveshare 7-inch MIPI-DSI LCD** to the 2-lane DSI FPC connector.
2. Ensure the **USB OTG Jumper** is set to **HOST**.
3. Plug your 2.4 GHz wireless keyboard dongle into the USB Type-A port.
4. Plug a USB flash drive (FAT32 or ext4 formatted) into a hub or second port.
5. Power the board via USB-C or external 5V header.
6. The screen will initialize into the 1024×600 terminal, and your keyboard will take direct control of the system without a PC attached.

---

## 📜 License & Acknowledgments

Distributed under the [MIT License](LICENSE).
Copyright (c) 2026 Valentin Stars.
Based on the RISC-V minimal NOMMU hypervisor research and Espressif ESP-IDF framework.
