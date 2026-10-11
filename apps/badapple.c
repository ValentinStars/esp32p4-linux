/*
 * badapple.c - Легковесный проигрыватель Bad Apple
 * для консоли и фреймбуфера ESP32-P4 RISC-V Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#define DEFAULT_FILE "/sdcard/badapple.bin"

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : DEFAULT_FILE;
    FILE *f = fopen(path, "r");
    if (!f) {
        /* Попробуем в /usbdrive, /mnt/sdcard или /tmp */
        f = fopen("/usbdrive/badapple.bin", "r");
        if (!f) f = fopen("/mnt/sdcard/badapple.bin", "r");
        if (!f) f = fopen("/tmp/badapple.bin", "r");
    }

    if (!f) {
        printf("Использование: badapple [путь_к_файлу]\n");
        printf("Файл %s не найден! Поместите badapple.bin на MicroSD карту.\n", path);
        return 1;
    }

    printf("\033[2J\033[H\033[?25l"); /* Очистить экран и скрыть курсор */
    fflush(stdout);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "---FRAME---", 11) == 0) {
            usleep(41666); /* ~24 FPS */
            printf("\033[H");
            fflush(stdout);
            continue;
        }
        fputs(line, stdout);
    }

    printf("\033[?25h\n"); /* Показать курсор */
    fclose(f);
    return 0;
}
