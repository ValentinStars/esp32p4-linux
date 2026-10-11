# Инструкция по сборке всех компонентов системы

В данном документе описывается полный процесс сборки:
1. Первая ступень / гипервизор (ESP-IDF v5.3+)
2. Дерево устройств (Device Tree Blob, DTB)
3. Корневая файловая система (Rootfs / Initramfs)
4. Ядро Linux (Linux Kernel 6.6+)

---

## 1. Требования к сборочному окружению

### Пакеты операционной системы (Host Linux)
Для хостовой системы (Arch / Manjaro / Ubuntu / Debian) необходимы следующие утилиты:
- `git`, `make`, `gcc`, `g++`, `bison`, `flex`, `bc`, `m4`
- `device-tree-compiler` (`dtc`)
- `python3`, `python3-pip`, `python3-venv`
- `cpio`, `rsync`, `xz`, `gzip`
- `esptool` (или установка через pip: `pip install esptool`)

### Кросс-компилятор RISC-V 32-bit
Для ядра и пользовательского пространства требуется GCC для целевой архитектуры `rv32ima` с ABI `ilp32`:
- Используется toolchain Bootlin: `riscv32-ilp32d--musl--stable-2024.05-1` либо `riscv64-unknown-linux-gnu-gcc` с поддержкой multilib (`-march=rv32ima -mabi=ilp32`).

---

## 2. Сборка первой ступени (ESP-IDF Firmware)

Первая ступень компилируется с помощью официального фреймворка Espressif ESP-IDF (версии 5.3 или выше):

```bash
cd bootloader
# Настройка целевого чипа ESP32-P4
idf.py set-target esp32p4

# Конфигурация параметров (опционально)
# idf.py menuconfig

# Сборка проекта
idf.py build
```

Результатом сборки в папке `bootloader/build/` являются:
- `bootloader/bootloader.bin` (Second stage bootloader ESP-IDF)
- `partition_table/partition-table.bin` (Таблица разделов)
- `esp32p4-linux-boot.bin` (Основное приложение гипервизора/SBI)

---

## 3. Сборка Device Tree Blob (DTB)

Дерево устройств описывает топологию гостевой системы (CPU, память, контроллер прерываний PLIC, таймер CLINT, UART NS16550A).

```bash
# Сборка для Sv32 MMU режима (основной)
dtc -I dts -O dtb -o kernel/dtb.bin kernel/uc-sv32.dts

# Либо через автоматический скрипт:
./scripts/build-dtb.sh
```

---

## 4. Сборка корневой файловой системы (Rootfs / Initramfs)

Файловая система упаковывается в формат архива `cpio` (стандартный Initramfs ядра Linux):

```bash
# Генерация дерева rootfs и создание cpio архива
./scripts/build-rootfs.sh
```

Скрипт формирует:
- `/etc/inittab` с запуском `rcS` и интерактивной getty консоли на `ttyS0`.
- `/etc/init.d/rcS` с автоматическим монтированием `/proc`, `/sys`, `/dev` (devtmpfs), `/tmp` (tmpfs), псевдотерминалов `pts` и общей памяти `shm`.
- `/etc/profile` с настройкой цветного приветствия, переменной `TERM=xterm-256color`, алиасов и путей `$PATH`.
- Набор тестовых скриптов в `/usr/bin/` (`memstress`, `nettest`, `sysinfo`).
- Консольные игры и утилиты.

---

## 5. Сборка ядра Linux

Ядро Linux 6.6+ собирается с использованием подготовленного `defconfig`:

```bash
cd linux-kernel
make ARCH=riscv CROSS_COMPILE=riscv32-linux-musl- defconfig
# Либо используем подготовленный конфиг:
cp ../kernel/defconfig .config

make ARCH=riscv CROSS_COMPILE=riscv32-linux-musl- \
     CC="riscv32-linux-musl-gcc -march=rv32ima_zicsr_zifencei -mabi=ilp32" \
     -j$(nproc) Image
```

Скомпилированный файл `arch/riscv/boot/Image` копируется в `kernel/Image`.
