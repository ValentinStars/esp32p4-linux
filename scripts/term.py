#!/usr/bin/env python3
"""
term.py - Удобный, надежный и чистый терминал для ESP32-P4 Linux
Не требует сторонних утилит, не сбрасывает плату через DTR/RTS,
корректно обрабатывает Backspace, стрелки и управляющие клавиши.

Выход из терминала: Ctrl + ]  (или Ctrl + Q)
"""

import sys
import os
import tty
import termios
import select
import serial

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    print(f"\033[1;36m=== Подключение к ESP32-P4 Linux: {port} @ {baud} ===\033[0m")
    print("\033[1;33m[Инструкция] Для выхода нажмите Ctrl + ] (или Ctrl + Q)\033[0m\n")

    try:
        # Важно: не дергаем DTR/RTS, чтобы не сбрасывать чип
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = baud
        ser.timeout = 0.05
        ser.dtr = False
        ser.rts = False
        ser.open()
    except Exception as e:
        print(f"\033[1;31mОшибка открытия порта {port}: {e}\033[0m")
        sys.exit(1)

    old_settings = termios.tcgetattr(sys.stdin)
    try:
        tty.setraw(sys.stdin.fileno())
        while True:
            r, _, _ = select.select([sys.stdin, ser], [], [])
            
            # Чтение из консоли пользователя и передача в плату
            if sys.stdin in r:
                ch = os.read(sys.stdin.fileno(), 1024)
                if not ch:
                    break
                # Ctrl+] (ASCII 29) или Ctrl+Q (ASCII 17) для выхода
                if b'\x1d' in ch or b'\x11' in ch:
                    break
                # Передаем напрямую в последовательный порт
                ser.write(ch)
                ser.flush()

            # Чтение из платы и вывод на экран
            if ser in r:
                data = ser.read(1024)
                if data:
                    os.write(sys.stdout.fileno(), data)
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
        ser.close()
        print("\n\033[1;32m[Терминал закрыт]\033[0m")

if __name__ == '__main__':
    main()
