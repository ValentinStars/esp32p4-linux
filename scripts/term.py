#!/usr/bin/env python3
"""
term.py - Профессиональный терминал для ESP32-P4 RISC-V Linux Cyberdeck.
Использует прямой POSIX tty интерфейс:
- НЕ дергает DTR/RTS (чип НИКОГДА не сбрасывается при подключении)
- Отключает HUPCL (нет сброса при закрытии)
- Корректно передает стрелки, Tab, Backspace, Ctrl+C, Ctrl+D
- Работает без сторонних зависимостей (чистый Python standard library)

Выход: Ctrl + ]  (или Ctrl + Q)
"""

import os
import sys
import termios
import tty
import select
import fcntl

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
    baud_rate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print(f"\033[1;36m=== ESP32-P4 Linux Terminal: {port} @ {baud_rate} ===\033[0m")
    print("\033[1;33m[Инструкция] Выход: Ctrl + ]  или  Ctrl + Q\033[0m\n")

    baud_map = {
        9600: termios.B9600,
        19200: termios.B19200,
        38400: termios.B38400,
        57600: termios.B57600,
        115200: termios.B115200,
        230400: termios.B230400,
        460800: termios.B460800,
        921600: termios.B921600,
    }
    speed = baud_map.get(baud_rate, termios.B115200)

    try:
        # Открываем порт без O_ASYNC и без дерганья DTR/RTS
        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    except Exception as e:
        print(f"\033[1;31mОшибка открытия {port}: {e}\033[0m")
        sys.exit(1)

    # Настройка параметров последовательного порта
    try:
        attrs = termios.tcgetattr(fd)
        attrs[4] = speed  # ispeed
        attrs[5] = speed  # ospeed
        attrs[0] = 0      # iflag (raw)
        attrs[1] = 0      # oflag (raw)
        # 8N1, CREAD, CLOCAL, без HUPCL (не сбрасывать чип при закрытии дескриптора)
        attrs[2] &= ~(termios.CSIZE | termios.PARENB | termios.CSTOPB | termios.HUPCL | termios.CRTSCTS)
        attrs[2] |= termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0      # lflag (raw, no echo, no signals)
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 1
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
    except Exception as e:
        print(f"\033[1;33mПредупреждение настройки порта: {e}\033[0m")

    # Перевод stdin терминала в raw режим
    old_stdin = termios.tcgetattr(sys.stdin)
    try:
        tty.setraw(sys.stdin.fileno())

        # Отправляем перевод строки для вызова шелла
        os.write(fd, b"\n")

        while True:
            rlist, _, _ = select.select([sys.stdin, fd], [], [])

            if sys.stdin in rlist:
                data = os.read(sys.stdin.fileno(), 1024)
                if not data:
                    break
                # Выход по Ctrl+] (0x1D) или Ctrl+Q (0x11)
                if b'\x1d' in data or b'\x11' in data:
                    break
                os.write(fd, data)

            if fd in rlist:
                try:
                    data = os.read(fd, 1024)
                    if data:
                        os.write(sys.stdout.fileno(), data)
                except BlockingIOError:
                    pass

    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_stdin)
        try:
            os.close(fd)
        except Exception:
            pass
        print("\n\033[1;32m[Терминал закрыт]\033[0m")

if __name__ == '__main__':
    main()
