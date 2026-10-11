/*
 * fastfetch.c - Реальная утилита сбора системной информации на C
 * для Waveshare ESP32-P4 RISC-V Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <ctype.h>

#define C "\033[1;36m"
#define R "\033[1;31m"
#define G "\033[1;32m"
#define Y "\033[1;33m"
#define B "\033[1;34m"
#define M "\033[1;35m"
#define W "\033[1;37m"
#define Z "\033[0m"

static void get_uptime(char *buf, size_t sz) {
    FILE *f = fopen("/proc/uptime", "r");
    double up = 0;
    if (f) {
        if (fscanf(f, "%lf", &up) == 1) {
            unsigned long sec = (unsigned long)up;
            unsigned long min = sec / 60;
            unsigned long hr = min / 60;
            if (hr > 0)
                snprintf(buf, sz, "%luh %lum", hr, min % 60);
            else if (min > 0)
                snprintf(buf, sz, "%lum %lus", min, sec % 60);
            else
                snprintf(buf, sz, "%lus", sec);
        }
        fclose(f);
    } else {
        snprintf(buf, sz, "unknown");
    }
}

static void get_memory(unsigned long *tot_mb, unsigned long *used_mb) {
    FILE *f = fopen("/proc/meminfo", "r");
    unsigned long free_kb = 0, tot_kb = 0;
    *tot_mb = 16;
    *used_mb = 2;
    if (f) {
        char line[128];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemTotal: %lu kB", &tot_kb) == 1) continue;
            if (sscanf(line, "MemFree: %lu kB", &free_kb) == 1) continue;
        }
        fclose(f);
        if (tot_kb > 0) {
            *tot_mb = tot_kb / 1024;
            *used_mb = (tot_kb >= free_kb) ? (tot_kb - free_kb) / 1024 : 0;
        }
    }
}

static int get_process_count(void) {
    int count = 0;
    DIR *d = opendir("/proc");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (isdigit(de->d_name[0])) count++;
        }
        closedir(d);
    }
    return count > 0 ? count : 1;
}

int main(int argc, char **argv) {
    struct utsname un;
    char kernel[64] = "6.9.0-uc-rv32ima";
    char uptime[32];
    unsigned long tot_mb = 16, used_mb = 2;
    int procs;

    if (uname(&un) == 0) {
        snprintf(kernel, sizeof(kernel), "%s %s", un.sysname, un.release);
    }

    get_uptime(uptime, sizeof(uptime));
    get_memory(&tot_mb, &used_mb);
    procs = get_process_count();

    printf("\n");
    printf("%s       _______       %sroot%s@%sesp32p4%s\n", C, W, Z, C, Z);
    printf("%s      / _____ \\      %s------------------------------------%s\n", C, W, Z);
    printf("%s     / /     \\ \\     %sOS:%s      Linux NOMMU (RV32IMA Native C)\n", C, C, Z);
    printf("%s    / /       \\ \\    %sHost:%s    Waveshare ESP32-P4-Module-DEV-KIT\n", C, C, Z);
    printf("%s   / /   %sRISC-V%s \\ \\   %sKernel:%s  %s\n", C, R, C, C, Z, kernel);
    printf("%s  / /             \\ \\  %sUptime:%s  %s\n", C, C, Z, uptime);
    printf("%s / /___      ______\\ \\ %sShell:%s   BusyBox ash\n", C, C, Z);
    printf("%s/_____/     /________/%sCPU:%s     ESP32-P4 Dual-Core RISC-V @ 400 MHz\n", C, C, Z);
    printf("                     %sMemory:%s  %lu MB / %lu MB PSRAM\n", C, Z, used_mb, tot_mb);
    printf("                     %sTasks:%s   %d processes\n", C, Z, procs);
    printf("\n");
    printf("   %s███%s███%s███%s███%s███%s███%s███%s\n", R, G, Y, B, M, C, W, Z);
    printf("\n");

    return 0;
}
