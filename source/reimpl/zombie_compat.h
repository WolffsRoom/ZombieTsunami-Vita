#ifndef ZOMBIE_TSUNAMI_COMPAT_H
#define ZOMBIE_TSUNAMI_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct zt_iovec {
    void *iov_base;
    size_t iov_len;
} zt_iovec;

unsigned int zt_alarm(unsigned int seconds);
int zt_chown(const char *path, unsigned int owner, unsigned int group);
int zt_dladdr(const void *address, void *info);
int zt_dup2(int oldfd, int newfd);
int zt_execl(const char *path, const char *arg, ...);
unsigned int zt_getegid(void);
unsigned int zt_geteuid(void);
unsigned int zt_getgid(void);
void *zt_getpwuid(unsigned int uid);
int zt_gettid(void);
unsigned int zt_getuid(void);
uint32_t zt_inet_addr(const char *address);
int zt_kill(int pid, int signal);
int zt_socketpair(int domain, int type, int protocol, int pair[2]);
int zt_writev(int fd, const zt_iovec *iov, int count);
void zt_glShaderBinary(void);

#endif
