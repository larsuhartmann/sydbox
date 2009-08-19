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

#include <assert.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

#include "trace.h"
#include "trace-util.h"

#ifdef HAVE_ASM_REG_H
#define fpq kernel_fpq
#define fq kernel_fq
#define fpu kernel_fpu
#include <asm/reg.h>
#undef fpq
#undef fq
#undef fpu
#endif // HAVE_ASM_REG_H

#define r_pc r_tpc
#undef PTRACE_GETREGS
#define PTRACE_GETREGS PTRACE_GETREGS64
#undef PTRACE_SETREGS
#define PTRACE_SETREGS PTRACE_SETREGS64

inline int trace_personality(pid_t pid G_GNUC_UNUSED)
{
    return 0;
/* FIXME: Why doesn't this work?! */
#if 0
    struct regs regs;
    unsigned long trap;

    if (0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))
        return -1;

    errno = 0;
    trap = ptrace(PTRACE_PEEKTEXT, pid, regs.r_pc, NULL);
    trap >>= 32;

    if (errno)
        return -1;

    switch (trap) {
        case 0x91d02010:
            /* Linux/SPARC syscall trap. */
            return 0;
        case 0x91d0206d:
            /* Linux/SPARC64 syscall trap. */
            return 1;
        default:
            g_warning("unknown system call trap %08lx %016lx\n", trap, regs.r_tpc);
            return -1;
    }
#endif
}

int trace_get_syscall(pid_t pid, long *scno)
{
    int save_errno;
    struct regs regs;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get syscall number for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    *scno = regs.r_g1;
    if (*scno == 0)
        *scno = regs.r_o0;

    return 0;
}

int trace_set_syscall(pid_t pid, long scno)
{
    int save_errno;
    struct regs regs;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to set syscall number to %ld for child %i: %s", scno, pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    regs.r_g1 = scno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_SETREGS, pid, &regs, NULL))) {
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
    struct regs regs;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get return value for child %i: %s", pid, g_strerror (errno));
        errno = save_errno;
        return -1;
    }

    if (regs.r_tstate & 0x1100000000UL)
        *res = -(regs.r_o0);
    else
        *res = regs.r_o0;

    return 0;
}

int trace_set_return(pid_t pid, long val)
{
    int save_errno;
    struct regs regs;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to set return for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    if (val < 0)
        regs.r_tstate |= 0x1100000000UL;
    else
        regs.r_tstate &= ~0x1100000000UL;

    regs.r_o0 = val;

    if (G_UNLIKELY(0 > ptrace(PTRACE_SETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to set return for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_get_arg(pid_t pid, int personality G_GNUC_UNUSED, int arg, long *res)
{
    int save_errno;
    struct regs regs;

    assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

    *res = *((&regs.r_o0) + arg);

    return 0;
}

char *trace_get_path(pid_t pid, int personality G_GNUC_UNUSED, int arg)
{
    int save_errno;
    long addr = 0;
    struct regs regs;

    assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", arg, g_strerror(errno));
        errno = save_errno;
        return NULL;
    }
    addr = regs.r_o0 + arg;

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

int trace_set_path(pid_t pid, int personality G_GNUC_UNUSED, int arg, const char *src, size_t len)
{
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;
    struct regs regs;

    assert(arg >= 0 && arg < MAX_ARGS);

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get address of argument %d: %s", arg, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    addr = regs.r_o0 + arg;

    n = 0;
    m = len / sizeof(long);

    while (n < m) {
        memcpy(u.x, src, sizeof(long));
        if (G_UNLIKELY(0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val))) {
            save_errno = errno;
            g_info("failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
        ++n;
        src += sizeof(long);
    }

    m = len % sizeof(long);
    if (0 != m) {
        errno = 0;
        u.val = ptrace(PTRACE_PEEKDATA, pid, addr + n * ADDR_MUL, 0);
        if (G_UNLIKELY(0 != errno)) {
            save_errno = errno;
            g_info("failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
        memcpy(u.x, src, m);
        if (G_UNLIKELY(0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val))) {
            save_errno = errno;
            g_info("failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, g_strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
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
    struct regs regs;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETREGS, pid, &regs, NULL))) {
        save_errno = errno;
        g_info("failed to get address of argument 1: %s", g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    addr = regs.r_o0 + 1;

    memset(&fakebuf, 0, sizeof(struct stat));
    fakebuf.st_mode = S_IFDIR;
    fakebuf.st_uid  = 0;
    fakebuf.st_gid  = 0;

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

