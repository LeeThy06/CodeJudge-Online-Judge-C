#ifndef OJ_PROTOCOL_H
#define OJ_PROTOCOL_H
#include <stddef.h>
#define OJ_PORT 5050
#define OJ_MAX_CODE (256 * 1024)
#define OJ_MAX_LINE 16384
int send_all(int fd, const void *buf, size_t len);
int recv_all(int fd, void *buf, size_t len);
int recv_line(int fd, char *buf, size_t cap);
#endif
