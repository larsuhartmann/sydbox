/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>

#include "trace.h"
#include "trace-util.h"

#define ORIG_ACCUM      (4 * ORIG_EAX)
#define ACCUM           (4 * EAX)
static const long syscall_args[1][MAX_ARGS] = {
    {4 * EBX, 4 * ECX, 4 * EDX, 4 * ESI, 4 * EDI, 4 * EBP}
};

inline int trace_personality(pid_t pid G_GNUC_UNUSED)
{
    return 0;
}

int trace_get_syscall(pid_t pid, long *scno)
{
    int save_errno;

    if (G_UNLIKELY(0 > upeek(pid, ORIG_ACCUM, scno))) {
        save_errno = errno;
        g_info("failed to get syscall number for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_set_syscall(pid_t pid, long scno)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_POKEUSER, pid, ORIG_ACCUM, scno))) {
        save_errno = errno;
        g_info("failed to set syscall number to %ld for child %i: %s", scno, pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_return(pid_t pid, long *res)
{
    int save_errno;

    if (G_UNLIKELY(0 > upeek(pid, ACCUM, res))) {
        save_errno = errno;
        g_info("failed to get return value for child %i: %s", pid, g_strerror (errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_set_return(pid_t pid, long val)
{
    int save_errno;

    if (G_UNLIKELY(0 != ptrace(PTRACE_POKEUSER, pid, ACCUM, val))) {
        save_errno = errno;
        g_info("ptrace(PTRACE_POKEUSER,%i,ACCUM,%ld) failed: %s", pid, val, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_arg(pid_t pid, int personality, int arg, long *res)
{
    int save_errno;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][arg], res))) {
        save_errno = errno;
        g_info("failed to get argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

char *trace_get_path(pid_t pid, int personality, int arg)
{
    int save_errno;
    long addr = 0;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][arg], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", arg, g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    char *buf = NULL;
    long len = PATH_MAX;
    for (;;) {
        buf = g_realloc(buf, len * sizeof(char));
        memset(buf, 0, len * sizeof(char));
        if (G_UNLIKELY(0 > umovestr(pid, addr, buf, len))) {
            g_free(buf);
            return NULL;
        }
        else if ('\0' == buf[len - 1])
            break;
        else
            len *= 2;
    }
    return buf;
}

int trace_fake_stat(pid_t pid, int personality)
{
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;
    struct stat fakebuf;

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][1], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", 1, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    memset(&fakebuf, 0, sizeof(struct stat));
    fakebuf.st_mode = S_IFBLK;

    long *fakeptr = (long *) &fakebuf;
    n = 0;
    m = sizeof(struct stat) / sizeof(long);
    while (n < m) {
        memcpy(u.x, fakeptr, sizeof(long));
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            g_info("failed to set argument 1 to %p for child %i: %s", (void *) fakeptr, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
        ++n;
        ++fakeptr;
    }

    m = sizeof(struct stat) % sizeof(long);
    if (0 != m) {
        memcpy(u.x, fakeptr, m);
        if (G_UNLIKELY(0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val))) {
            save_errno = errno;
            g_info("failed to set argument 1 to %p for child %i: %s", (void *) fakeptr, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
}

int trace_decode_socketcall(pid_t pid, int personality)
{
    int save_errno;
    long addr;

     if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][0], &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument 0: %s", g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return addr;
}

char *trace_get_addr(pid_t pid, int personality, bool decode G_GNUC_UNUSED, int *family)
{
    int save_errno;
    long args;
    unsigned int addr, addrlen;
    union {
        char pad[128];
        struct sockaddr sa;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa6;
    } addrbuf;
    char ip[100];

    if (G_UNLIKELY(0 > upeek(pid, syscall_args[personality][1], &args))) {
        save_errno = errno;
        g_info("failed to get address of argument 1: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    args += ADDR_MUL; // skip the first argument which is sockfd.
    if (umove(pid, args, &addr) < 0) {
        save_errno = errno;
        g_info("failed to decode argument 1: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }
    args += ADDR_MUL;
    if (umove(pid, args, &addrlen) < 0) {
        save_errno = errno;
        g_info("failed to decode argument 2: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    if (addrlen < 2 || addrlen > sizeof(addrbuf))
        addrlen = sizeof(addrbuf);

    memset(&addrbuf, 0, sizeof(addrbuf));
    if (umoven(pid, addr, addrbuf.pad, addrlen) < 0) {
        save_errno = errno;
        g_info("failed to get socket address: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }
    addrbuf.pad[sizeof(addrbuf.pad) - 1] = '\0';

    if (family != NULL)
        *family = addrbuf.sa.sa_family;

    switch (addrbuf.sa.sa_family) {
        case AF_UNIX:
            /* We don't care about unix sockets for now */
            return g_strdup("unix");
        case AF_INET:
            if (!inet_ntop(AF_INET, &addrbuf.sa_in.sin_addr, ip, sizeof(ip))) {
                save_errno = errno;
                g_info("inet_ntop() failed: %s", g_strerror(errno));
                errno = save_errno;
                return NULL;
            }
            return g_strdup(ip);
        case AF_INET6:
            if (!inet_ntop(AF_INET6, &addrbuf.sa6.sin6_addr, ip, sizeof(ip))) {
                save_errno = errno;
                g_info("inet_ntop() failed: %s", g_strerror(errno));
                errno = save_errno;
                return NULL;
            }
            return g_strdup(ip);
        default:
            return g_strdup("other");
    }
}

