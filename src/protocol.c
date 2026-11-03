#include "protocol.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

int send_all(int fd, const void *buf, size_t len) {
    const char *p = buf; size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}
int recv_all(int fd, void *buf, size_t len) {
    char *p = buf; size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, p + got, len - got, 0);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        if (n == 0) return -1;
        got += (size_t)n;
    }
    return 0;
}
int recv_line(int fd, char *buf, size_t cap) {
    if (cap < 2) return -1;
    size_t i = 0;
    while (i + 1 < cap) {
        char c; ssize_t n = recv(fd, &c, 1, 0);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
        if (n == 0) return -1;
        if (c == '\n') break;
        if (c != '\r') buf[i++] = c;
    }
    buf[i] = '\0';
    return (int)i;
}
