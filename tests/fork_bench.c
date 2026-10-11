/*
 * fork_bench.c - Бенчмарк системных вызовов fork, vfork и работы с памятью
 * для ESP32-P4 RISC-V Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_CYCLES 20

int main(int argc, char **argv)
{
    printf("=========================================\n");
    printf("  ESP32-P4 RISC-V Fork & Memory Benchmark\n");
    printf("=========================================\n");

    int i;
    for (i = 0; i < NUM_CYCLES; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return 1;
        } else if (pid == 0) {
            /* Дочерний процесс */
            char *buf = malloc(16 * 1024); // 16 KB
            if (buf) {
                memset(buf, 0xAA, 16 * 1024);
                free(buf);
            }
            _exit(0);
        } else {
            /* Родительский процесс */
            int status = 0;
            waitpid(pid, &status, 0);
        }
    }

    printf("✓ Успешно выполнено %d циклов fork() + malloc(16KB)!\n", NUM_CYCLES);
    printf("=========================================\n");
    return 0;
}
