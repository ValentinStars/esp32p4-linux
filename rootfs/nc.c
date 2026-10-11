/*
 * nc.c - Netcat for RISC-V Linux (ESP32-P4)
 * Supports client and server modes (-l -p <port>), TCP and UDP.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define BUF_SIZE 4096

static void usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  Client mode: %s [-u] [-v] <host> <port>\n", prog);
    fprintf(stderr, "  Server mode: %s -l -p <port> [-u] [-v]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -l       Listen mode (server)\n");
    fprintf(stderr, "  -p PORT  Local port to listen on or bind\n");
    fprintf(stderr, "  -u       Use UDP instead of default TCP\n");
    fprintf(stderr, "  -v       Verbose output\n");
    fprintf(stderr, "  -h       Show this help\n");
    exit(1);
}

static void relay(int fd1, int fd2)
{
    char buf[BUF_SIZE];
    fd_set readfds;
    int maxfd = (fd1 > fd2) ? fd1 : fd2;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd1, &readfds);
        FD_SET(fd2, &readfds);

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // fd1 -> fd2
        if (FD_ISSET(fd1, &readfds)) {
            ssize_t n = read(fd1, buf, sizeof(buf));
            if (n <= 0) break;
            ssize_t written = 0;
            while (written < n) {
                ssize_t w = write(fd2, buf + written, n - written);
                if (w <= 0) break;
                written += w;
            }
        }

        // fd2 -> fd1
        if (FD_ISSET(fd2, &readfds)) {
            ssize_t n = read(fd2, buf, sizeof(buf));
            if (n <= 0) break;
            ssize_t written = 0;
            while (written < n) {
                ssize_t w = write(fd1, buf + written, n - written);
                if (w <= 0) break;
                written += w;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int listen_mode = 0;
    int udp_mode = 0;
    int verbose = 0;
    int port = 0;
    char *host = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            listen_mode = 1;
        } else if (strcmp(argv[i], "-u") == 0) {
            udp_mode = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
        } else if (argv[i][0] != '-') {
            if (!host) host = argv[i];
            else if (!port) port = atoi(argv[i]);
        }
    }

    signal(SIGPIPE, SIG_IGN);

    int sock_type = udp_mode ? SOCK_DGRAM : SOCK_STREAM;

    if (listen_mode) {
        if (port <= 0) {
            fprintf(stderr, "Error: Specify a valid port with -p <port>\n");
            return 1;
        }

        int sfd = socket(AF_INET, sock_type, 0);
        if (sfd < 0) {
            perror("socket");
            return 1;
        }

        int opt = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        if (bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("bind");
            close(sfd);
            return 1;
        }

        if (udp_mode) {
            if (verbose) fprintf(stderr, "Listening on UDP port %d...\n", port);
            relay(STDIN_FILENO, sfd);
            close(sfd);
            return 0;
        }

        if (listen(sfd, 1) < 0) {
            perror("listen");
            close(sfd);
            return 1;
        }

        if (verbose) fprintf(stderr, "Listening on 0.0.0.0:%d (TCP)...\n", port);

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_len);
        if (cfd < 0) {
            perror("accept");
            close(sfd);
            return 1;
        }

        if (verbose) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            fprintf(stderr, "Connection accepted from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        }

        relay(STDIN_FILENO, cfd);
        close(cfd);
        close(sfd);
    } else {
        if (!host || port <= 0) {
            usage(argv[0]);
        }

        struct hostent *server = gethostbyname(host);
        if (!server) {
            fprintf(stderr, "Error: Could not resolve hostname '%s'\n", host);
            return 1;
        }

        int sfd = socket(AF_INET, sock_type, 0);
        if (sfd < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(port);

        if (verbose) fprintf(stderr, "Connecting to %s:%d...\n", host, port);

        if (connect(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("connect");
            close(sfd);
            return 1;
        }

        if (verbose) fprintf(stderr, "Connected!\n");

        relay(STDIN_FILENO, sfd);
        close(sfd);
    }

    return 0;
}
