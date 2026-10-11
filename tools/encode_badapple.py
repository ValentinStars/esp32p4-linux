#!/usr/bin/env python3
"""
encode_badapple.py - Кодировщик видео Bad Apple (MP4) для ESP32-P4 Linux
Поддерживает два режима:
1. 'fb'    - Сжатый бинарный поток для вывода в Linux Framebuffer (/dev/fb0)
2. 'ascii' - Компактный текстовый поток для TTY терминала на экране
"""

import sys
import os
import subprocess
import argparse

def main():
    parser = argparse.ArgumentParser(description="Bad Apple Video Encoder for ESP32-P4")
    parser.add_argument("input", help="Путь к исходному mp4 файлу")
    parser.add_argument("--mode", choices=["ascii", "fb"], default="ascii", help="Режим вывода (ascii или fb)")
    parser.add_argument("--width", type=int, default=80, help="Ширина в символах или пикселях")
    parser.add_argument("--height", type=int, default=24, help="Высота в строках или пикселях")
    parser.add_argument("--fps", type=int, default=24, help="Частота кадров")
    parser.add_argument("--output", "-o", default="badapple.bin", help="Имя выходного файла")
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Ошибка: файл {args.input} не найден!")
        sys.exit(1)

    print(f"Кодирование {args.input} в режим '{args.mode}' ({args.width}x{args.height} @ {args.fps}fps)...")

    if args.mode == "ascii":
        # Извлекаем кадры через ffmpeg в градации серого
        cmd = [
            "ffmpeg", "-i", args.input,
            "-vf", f"scale={args.width}:{args.height},fps={args.fps}",
            "-f", "rawvideo", "-pix_fmt", "gray", "-"
        ]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        chars = " .:-=+*#%@"
        frame_size = args.width * args.height
        total_frames = 0

        with open(args.output, "w", encoding="utf-8") as out_f:
            while True:
                raw = proc.stdout.read(frame_size)
                if not raw or len(raw) < frame_size:
                    break
                # Очистка экрана и перемещение курсора в начало кадра
                out_f.write("\033[H")
                for y in range(args.height):
                    row = raw[y * args.width : (y + 1) * args.width]
                    line = "".join(chars[b * (len(chars) - 1) // 255] for b in row)
                    out_f.write(line + "\n")
                out_f.write("---FRAME---\n")
                total_frames += 1

        print(f"✓ Успешно закодировано {total_frames} кадров в {args.output}!")

    elif args.mode == "fb":
        # Извлекаем в сырой RGB565 / монохромный поток
        cmd = [
            "ffmpeg", "-i", args.input,
            "-vf", f"scale={args.width}:{args.height},fps={args.fps}",
            "-f", "rawvideo", "-pix_fmt", "rgb565le", args.output
        ]
        subprocess.run(cmd, check=True)
        print(f"✓ Сырой видеопоток RGB565 сохранен в {args.output}!")

if __name__ == "__main__":
    main()
