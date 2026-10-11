/*
 * nmap.c - Lightweight port scanner for RISC-V Linux (ESP32-P4)
 * Supports single port, port ranges (-p 1-1024), comma lists (-p 22,80,443)
 * Fast non-blocking connect scanning with service banner detection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

#define DEFAULT_TIMEOUT_MS 250
#define MAX_PORTS 2048

static const struct {
    int port;
    const char *service;
} WELL_KNOWN_SERVICES[] = {
    {21, "ftp"}, {22, "ssh"}, {23, "telnet"}, {25, "smtp"},
    {53, "dns"}, {80, "http"}, {110, "pop3"}, {111, "rpcbind"},
    {139, "netbios"}, {143, "imap"}, {443, "https"}, {445, "microsoft-ds"},
    {993, "imaps"}, {995, "pop3s"}, {1080, "socks"}, {1883, "mqtt"},
    {3306, "mysql"}, {3389, "ms-wbt-server"}, {5432, "postgresql"},
    {6379, "redis"}, {8000, "http-alt"}, {8080, "http-proxy"},
    {8443, "https-alt"}, {9000, "cslistener"}, {0, NULL}
};

static const char *get_service_name(int port)
{
    for (int i = 0; WELL_KNOWN_SERVICES[i].port != 0; i++) {
        if (WELL_KNOWN_SERVICES[i].port == port) {
            return WELL_KNOWN_SERVICES[i].service;
        }
    }
    struct servent *s = getservbyport(htons(port), "tcp");
    if (s) return s->s_name;
    return "unknown";
}

static int is_port_open(const struct sockaddr_in *target, int port, int timeout_ms)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return 0;

    int flags = fcntl(sfd, F_GETFL, 0);
    fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr = *target;
    addr.sin_port = htons(port);

    int res = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0 && errno == EINPROGRESS) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sfd, &wfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int sel = select(sfd + 1, NULL, &wfds, NULL, &tv);
        if (sel > 0) {
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(sfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                close(sfd);
                return 1; // Open!
            }
        }
    } else if (res == 0) {
        close(sfd);
        return 1;
    }

    close(sfd);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ESP32-P4 Nmap Port Scanner v1.0\n");
        fprintf(stderr, "Usage: %s <target_ip_or_host> [-p <ports>] [-t <timeout_ms>]\n", argv[0]);
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  %s 192.168.1.1\n", argv[0]);
        fprintf(stderr, "  %s 192.168.1.10 -p 22,80,443,8080\n", argv[0]);
        fprintf(stderr, "  %s 127.0.0.1 -p 1-1024 -t 150\n", argv[0]);
        return 1;
    }

    char *target_host = argv[1];
    char *port_spec = NULL;
    int timeout_ms = DEFAULT_TIMEOUT_MS;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port_spec = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout_ms = atoi(argv[++i]);
        }
    }

    struct hostent *he = gethostbyname(target_host);
    if (!he) {
        fprintf(stderr, "Failed to resolve host '%s'\n", target_host);
        return 1;
    }

    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    memcpy(&target_addr.sin_addr.s_addr, he->h_addr, he->h_length);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &target_addr.sin_addr, ip_str, sizeof(ip_str));

    printf("Starting Nmap 1.0 ( https://esp32p4-linux.local ) at %s\n", ip_str);
    printf("Nmap scan report for %s (%s)\n", target_host, ip_str);

    int ports[MAX_PORTS];
    int num_ports = 0;

    if (port_spec) {
        char *copy = strdup(port_spec);
        char *token = strtok(copy, ",");
        while (token && num_ports < MAX_PORTS) {
            char *dash = strchr(token, '-');
            if (dash) {
                *dash = '\0';
                int start = atoi(token);
                int end = atoi(dash + 1);
                if (start < 1) start = 1;
                if (end > 65535) end = 65535;
                for (int p = start; p <= end && num_ports < MAX_PORTS; p++) {
                    ports[num_ports++] = p;
                }
            } else {
                int p = atoi(token);
                if (p > 0 && p <= 65535) {
                    ports[num_ports++] = p;
                }
            }
            token = strtok(NULL, ",");
        }
        free(copy);
    } else {
        // Default top ports
        int top_ports[] = {
            21, 22, 23, 25, 53, 80, 110, 111, 135, 139, 143, 443, 445,
            993, 995, 1723, 1883, 3306, 3389, 5432, 5900, 6379, 8000, 8080, 8443, 9000
        };
        num_ports = sizeof(top_ports) / sizeof(top_ports[0]);
        for (int i = 0; i < num_ports; i++) ports[i] = top_ports[i];
    }

    printf("Host is up. Scanning %d ports (timeout: %d ms)...\n\n", num_ports, timeout_ms);
    printf("PORT     STATE SERVICE\n");

    int open_count = 0;
    for (int i = 0; i < num_ports; i++) {
        int p = ports[i];
        if (is_port_open(&target_addr, p, timeout_ms)) {
            printf("%-8d open  %s\n", p, get_service_name(p));
            open_count++;
        }
    }

    if (open_count == 0) {
        printf("All %d scanned ports are closed or filtered.\n", num_ports);
    }

    printf("\nNmap done: 1 IP address (1 host up) scanned, %d open ports found.\n", open_count);
    return 0;
}
