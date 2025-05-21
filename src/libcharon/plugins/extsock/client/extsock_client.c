// extsock_client.c: CLI client for strongSwan extsock plugin
// Usage:
//   ./extsock_client apply-config <jsonfile> [--wait-events]
//   ./extsock_client start-dpd <ike_sa_name> [--wait-events]
//   ./extsock_client monitor-events
//
// Requires: cJSON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <cJSON.h>

#define SOCKET_PATH "/tmp/strongswan_extsock.sock"
#define BUF_SIZE 4096

void print_usage(const char *prog) {
    printf("Usage:\n");
    printf("  %s apply-config <jsonfile> [--wait-events]\n", prog);
    printf("  %s start-dpd <ike_sa_name> [--wait-events]\n", prog);
    printf("  %s monitor-events\n", prog);
}

// Connect to extsock plugin socket
int connect_socket() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }
    return fd;
}

// Read file into buffer (returns malloc'd buffer, must free)
char* read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, len, f);
    if (nread != len) {
        fprintf(stderr, "[error] Failed to read entire file: %s\n", filename);
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[len] = '\0';
    fclose(f);
    return buf;
}

// Print events from socket (blocking)
void monitor_events(int fd) {
    char buf[BUF_SIZE];
    while (1) {
        ssize_t len = read(fd, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = '\0';
            cJSON *json = cJSON_Parse(buf);
            if (json) {
                cJSON *event = cJSON_GetObjectItem(json, "event");
                if (event && cJSON_IsString(event)) {
                    printf("[event] %s\n", event->valuestring);
                    printf("[json]  %s\n", buf);
                } else {
                    printf("[json]  %s\n", buf);
                }
                cJSON_Delete(json);
            } else {
                printf("[raw]   %s\n", buf);
            }
        } else if (len == 0) {
            printf("[info] Connection closed by server.\n");
            break;
        } else {
            perror("read");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    bool wait_events = false;
    if ((argc >= 4 && strcmp(argv[argc-1], "--wait-events") == 0) ||
        (argc == 3 && strcmp(argv[2], "--wait-events") == 0)) {
        wait_events = true;
        argc--; // treat --wait-events as not part of main args
    }
    int fd = connect_socket();
    if (fd < 0) return 1;

    if (strcmp(argv[1], "apply-config") == 0 && argc >= 3) {
        // Read JSON file
        char *json_str = read_file(argv[2]);
        if (!json_str) { close(fd); return 1; }
        // Compose command
        size_t cmd_len = strlen("APPLY_CONFIG ") + strlen(json_str) + 1;
        char *cmd = malloc(cmd_len);
        snprintf(cmd, cmd_len, "APPLY_CONFIG %s", json_str);
        if (write(fd, cmd, strlen(cmd)) < 0) {
            perror("write");
            free(cmd); free(json_str); close(fd); return 1;
        }
        printf("[cmd] Sent APPLY_CONFIG\n");
        free(cmd); free(json_str);
        if (wait_events) {
            monitor_events(fd);
        }
    } else if (strcmp(argv[1], "start-dpd") == 0 && argc >= 3) {
        // Compose command
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "START_DPD %s", argv[2]);
        if (write(fd, cmd, strlen(cmd)) < 0) {
            perror("write");
            close(fd); return 1;
        }
        printf("[cmd] Sent START_DPD %s\n", argv[2]);
        if (wait_events) {
            monitor_events(fd);
        }
    } else if (strcmp(argv[1], "monitor-events") == 0) {
        monitor_events(fd);
    } else {
        print_usage(argv[0]);
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
} 