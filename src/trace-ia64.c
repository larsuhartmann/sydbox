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

#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>

#include "trace.h"
#include "trace-util.h"

#include <asm/ptrace_offsets.h>
#include <asm/rse.h>
#define ORIG_ACCUM      (PT_R15)
#define ACCUM           (PT_R10)

static int upeek_ia64(pid_t pid, int narg, long *res)
{
    unsigned long *out0, cfm, sof, sol;
    long rbs_end;

    if (0 > upeek(pid, PT_AR_BSP, &rbs_end))
        return -1;
    if (0 > upeek(pid, PT_CFM, (long *) &cfm))
        return -1;

    sof = (cfm >> 0) & 0x7f;
    sol = (cfm >> 7) & 0x7f;
    out0 = ia64_rse_skip_regs((unsigned long *) rbs_end, -sof + sol);

    return umoven(pid, (unsigned long) ia64_rse_skip_regs(out0, narg), (char *) res, sizeof(long));
}

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
    long r8, r10;

    r8 = -val;
    r10 = val ? -1 : 0;
    if (G_UNLIKELY(0 != ptrace(PTRACE_POKEUSER, pid, PT_R8, r8))) {
        save_errno = errno;
        g_info("ptrace(PTRACE_POKEUSER,%i,PT_R8,%ld) failed: %s", pid, val, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    if (G_UNLIKELY(0 != ptrace(PTRACE_POKEUSER, pid, PT_R10, r10))) {
        save_errno = errno;
        g_info("ptrace(PTRACE_POKEUSER,%i,PT_R10,%ld) failed: %s", pid, val, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_arg(pid_t pid, int personality G_GNUC_UNUSED, int arg, long *res)
{
    int save_errno;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek_ia64(pid, arg, res))) {
        save_errno = errno;
        g_info("failed to get argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

char *trace_get_path(pid_t pid, int personality G_GNUC_UNUSED, int arg)
{
    int save_errno;
    long addr = 0;

    g_assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > upeek_ia64(pid, arg, &addr))) {
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

int trace_fake_stat(pid_t pid, int personality G_GNUC_UNUSED)
{
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;
    struct stat fakebuf;

    if (G_UNLIKELY(0 > upeek_ia64(pid, 1, &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", 1, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    memset(&fakebuf, 0, sizeof(struct stat));
    fakebuf.st_mode = S_IFCHR | (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH);
    fakebuf.st_rdev = 259; // /dev/null
    fakebuf.st_mtime = -842745600; // ;)

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

int trace_decode_socketcall(pid_t pid, int personality G_GNUC_UNUSED)
{
    int save_errno;
    long addr;

    if (G_UNLIKELY(0 > upeek_ia64(pid, 0, &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument 0: %s", g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return addr;
}

char *trace_get_addr(pid_t pid, int personality G_GNUC_UNUSED, bool decode G_GNUC_UNUSED, int *family, int *port)
{
    int save_errno;
    long addr, addrlen;
    union {
        char pad[128];
        struct sockaddr sa;
        struct sockaddr_un sa_un;
        struct sockaddr_in sa_in;
        struct sockaddr_in6 sa6;
    } addrbuf;
    char ip[100];

    if (G_UNLIKELY(0 > upeek_ia64(pid, 1, &addr))) {
        save_errno = errno;
        g_info("failed to get address of argument 1: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }
    if (G_UNLIKELY(0 > upeek_ia64(pid, 2, &addrlen))) {
        save_errno = errno;
        g_info("failed to get address of argument 2: %s", g_strerror(errno));
        errno = save_errno;
        return NULL;
    }

    if (addrlen < 2 || (unsigned long)addrlen > sizeof(addrbuf))
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
    if (port != NULL)
        *port = -1;

    switch (addrbuf.sa.sa_family) {
        case AF_UNIX:
            return g_strdup(addrbuf.sa_un.sun_path);
        case AF_INET:
            if (port != NULL)
                *port = ntohs(addrbuf.sa_in.sin_port);
            if (!inet_ntop(AF_INET, &addrbuf.sa_in.sin_addr, ip, sizeof(ip))) {
                save_errno = errno;
                g_info("inet_ntop() failed: %s", g_strerror(errno));
                errno = save_errno;
                return NULL;
            }
            return g_strdup(ip);
        case AF_INET6:
            if (port != NULL)
                *port = ntohs(addrbuf.sa6.sin6_port);
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

