#include "reimpl/zombie_compat.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/unistd.h>

#include <psp2/kernel/threadmgr.h>

#include "utils/logger.h"

unsigned int zt_alarm(unsigned int seconds) {
    (void)seconds;
    return 0;
}

int zt_chown(const char *path, unsigned int owner, unsigned int group) {
    (void)path;
    (void)owner;
    (void)group;
    return 0;
}

int zt_dladdr(const void *address, void *info) {
    (void)address;
    (void)info;
    return 0;
}

int zt_dup2(int oldfd, int newfd) {
    (void)oldfd;
    (void)newfd;
    errno = ENOSYS;
    return -1;
}

int zt_execl(const char *path, const char *arg, ...) {
    (void)path;
    (void)arg;
    errno = ENOSYS;
    return -1;
}

unsigned int zt_getegid(void) { return 0; }
unsigned int zt_geteuid(void) { return 0; }
unsigned int zt_getgid(void) { return 0; }
void *zt_getpwuid(unsigned int uid) { (void)uid; return NULL; }
int zt_gettid(void) { return sceKernelGetThreadId(); }
unsigned int zt_getuid(void) { return 0; }

uint32_t zt_inet_addr(const char *address) {
    uint8_t octets[4];
    unsigned int a, b, c, d;
    if (!address || sscanf(address, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 ||
        a > 255 || b > 255 || c > 255 || d > 255) {
        return 0xffffffffu;
    }
    octets[0] = (uint8_t)a;
    octets[1] = (uint8_t)b;
    octets[2] = (uint8_t)c;
    octets[3] = (uint8_t)d;
    return ((uint32_t)octets[0]) |
           ((uint32_t)octets[1] << 8) |
           ((uint32_t)octets[2] << 16) |
           ((uint32_t)octets[3] << 24);
}

int zt_kill(int pid, int signal) {
    (void)pid;
    (void)signal;
    return 0;
}

int zt_socketpair(int domain, int type, int protocol, int pair[2]) {
    (void)domain;
    (void)type;
    (void)protocol;
    if (pair) {
        pair[0] = -1;
        pair[1] = -1;
    }
    errno = ENOSYS;
    return -1;
}

int zt_writev(int fd, const zt_iovec *iov, int count) {
    int total = 0;
    for (int i = 0; i < count; ++i) {
        int written = write(fd, iov[i].iov_base, iov[i].iov_len);
        if (written < 0) return total > 0 ? total : -1;
        total += written;
        if ((size_t)written != iov[i].iov_len) break;
    }
    return total;
}

void zt_glShaderBinary(void) {
    l_warn("glShaderBinary is unsupported; falling back to source shaders.");
}
