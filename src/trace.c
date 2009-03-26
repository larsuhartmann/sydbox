/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 * Based in part upon strace which is:
 *  Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 *  Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 *  Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 *  Copyright (c) 1996-1999 Wichert Akkerman <wichert@cistron.nl>
 *  Copyright (c) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *                     Linux for s390 port by D.J. Barrow
 *                    <barrow_dj@mail.yahoo.com,djbarrow@de.ibm.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "util.h"
#include "trace.h"
#include "syscall.h"

#define ADDR_MUL        ((64 == __WORDSIZE) ? 8 : 4)
#define MAX_ARGS        6
#if defined(I386)
#define ORIG_ACCUM      (4 * ORIG_EAX)
#define ACCUM           (4 * EAX)
static const long syscall_args[MAX_ARGS] = {4 * EBX, 4 * ECX, 4 * EDX, 4 * ESI, 4 * EDI, 4 * EBP};
#elif defined(X86_64)
#define ORIG_ACCUM      (8 * ORIG_RAX)
#define ACCUM           (8 * RAX)
static const long syscall_args[MAX_ARGS] = {8 * RDI, 8 * RSI, 8 * RDX, 8 * R10, 8 * R8, 8 * R9};
#endif

#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
static int umovestr(pid_t pid, long addr, char *dest, size_t len) {
    int n, m, save_errno;
    int started = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;

    if (addr & (sizeof(long) -1)) {
        // addr not a multiple of sizeof(long)
        n = addr - (addr & -sizeof(long)); // residue
        addr &= -sizeof(long); // residue
        errno = 0;
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (0 != errno) {
            if (started && (EPERM == errno || EIO == errno)) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (0 != addr && EIO != errno) {
                save_errno = errno;
                LOGV("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, strerror(errno));
                errno = save_errno;
            }
            return -1;
        }
        started = 1;
        memcpy(dest, &u.x[n], m = MIN(sizeof(long) - n, len));
        while (n & (sizeof(long) - 1))
            if ('\0' == u.x[n++])
                return 0;
        addr += sizeof(long), dest += m, len -= m;
    }
    while (len > 0) {
        errno = 0;
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (0 != errno) {
            if (started && (EPERM == errno || EIO == errno)) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (0 != addr && EIO != errno) {
                save_errno = errno;
                LOGV("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, strerror(errno));
                errno = save_errno;
            }
            return -1;
        }
        started = 1;
        memcpy(dest, u.x, m = MIN(sizeof(long), len));
        for (unsigned int i = 0; i < sizeof(long); i++)
            if ('\0' == u.x[i])
                return 0;
        addr += sizeof(long), dest += m, len -= m;
    }
    return 0;
}

unsigned int trace_event(int status) {
    if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        if (SIGSTOP == sig)
            return E_STOP;
        else if ((SIGTRAP | 0x80) == sig)
            return E_SYSCALL;

        switch (status) {
            case SIGTRAP | PTRACE_EVENT_FORK << 8:
                return E_FORK;
            case SIGTRAP | PTRACE_EVENT_VFORK << 8:
                return E_VFORK;
            case SIGTRAP | PTRACE_EVENT_CLONE << 8:
                return E_CLONE;
            case SIGTRAP | PTRACE_EVENT_EXEC << 8:
                return E_EXEC;
            default:
                return E_GENUINE;
        }
    }
    else if (WIFEXITED(status))
        return E_EXIT;
    else if (WIFSIGNALED(status))
        return E_EXIT_SIGNAL;

    return E_UNKNOWN;
}

int trace_me(void) {
    if (0 > ptrace(PTRACE_TRACEME, 0, NULL, NULL)) {
        int save_errno = errno;
        LOGV("Failed to set tracing: %s", strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_setup(pid_t pid) {
    // Setup ptrace options
    LOGD("Setting tracing options for child %i", pid);
    if (0 > ptrace(PTRACE_SETOPTIONS, pid, NULL,
                    PTRACE_O_TRACESYSGOOD
                    | PTRACE_O_TRACECLONE
                    | PTRACE_O_TRACEFORK
                    | PTRACE_O_TRACEVFORK
                    | PTRACE_O_TRACEEXEC)) {
        int save_errno = errno;
        LOGV("Setting tracing options failed for child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_kill(pid_t pid) {
    if (0 > ptrace(PTRACE_KILL, pid, NULL, NULL) && ESRCH != errno) {
        int save_errno = errno;
        LOGV("Failed to kill child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_cont(pid_t pid) {
    if (0 > ptrace(PTRACE_CONT, pid, NULL, NULL)) {
        int save_errno = errno;
        LOGV("Failed to continue child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_syscall(pid_t pid, int data) {
    if (0 > ptrace(PTRACE_SYSCALL, pid, NULL, data)) {
        int save_errno = errno;
        LOGV("Failed to resume child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_geteventmsg(pid_t pid, void *data) {
    if (0 > ptrace(PTRACE_GETEVENTMSG, pid, NULL, data)) {
        int save_errno = errno;
        LOGV("Failed to get event message of child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

static int trace_peek(pid_t pid, long off, long *res) {
    long val;

    errno = 0;
    val = ptrace(PTRACE_PEEKUSER, pid, off, NULL);
    if (-1 == val && 0 != errno) {
        int save_errno = errno;
        LOGV("ptrace(PTRACE_PEEKUSER,%d,%lu,NULL) failed: %s", pid, off, strerror(errno));
        errno = save_errno;
        return -1;
    }
    *res = val;
    return 0;
}

int trace_get_arg(pid_t pid, int arg, long *res) {
    assert(arg >= 0 && arg < MAX_ARGS);

    if (0 > trace_peek(pid, syscall_args[arg], res)) {
        int save_errno = errno;
        LOGV("Failed to get argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_get_syscall(pid_t pid, long *syscall) {
    if (0 > trace_peek(pid, ORIG_ACCUM, syscall)) {
        int save_errno = errno;
        LOGV("Failed to get syscall number for child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_set_syscall(pid_t pid, long syscall) {
    if (0 > ptrace(PTRACE_POKEUSER, pid, ORIG_ACCUM, syscall)) {
        int save_errno = errno;
        LOGV("Failed to set syscall number to %ld for child %i: %s", syscall, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_get_return(pid_t pid, long *res) {
    if (0 > trace_peek(pid, ACCUM, res)) {
        int save_errno = errno;
        LOGV("Failed to get return value for child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_set_return(pid_t pid, long val) {
    if (0 != ptrace(PTRACE_POKEUSER, pid, ACCUM, val)) {
        int save_errno = errno;
        LOGV("ptrace(PTRACE_POKEUSER,%i,ACCUM,%ld) failed: %s", pid, val, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

char *trace_get_string(pid_t pid, int arg) {
    int save_errno;
    long addr = 0;

    assert(arg >= 0 && arg < MAX_ARGS);
    if (0 > trace_peek(pid, syscall_args[arg], &addr)) {
        save_errno = errno;
        LOGV("Failed to get address of argument %d: %s", arg, strerror(errno));
        errno = save_errno;
        return NULL;
    }

    char *buf = NULL;
    long len = PATH_MAX;
    for (;;) {
        buf = xrealloc(buf, len * sizeof(char));
        memset(buf, 0, len * sizeof(char));
        if (0 > umovestr(pid, addr, buf, len)) {
            free(buf);
            return NULL;
        }
        else if ('\0' == buf[len - 1])
            break;
        else
            len *= 2;
    }
    return buf;
}

int trace_set_string(pid_t pid, int arg, const char *src, size_t len) {
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;

    assert(arg >= 0 && arg < MAX_ARGS);
    if (0 > trace_peek(pid, syscall_args[arg], &addr)) {
        save_errno = errno;
        LOGV("Failed to get address of argument %d for child %i: %s", arg, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

    n = 0;
    m = len / sizeof(long);

    while (n < m) {
        memcpy(u.x, src, sizeof(long));
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            LOGV("Failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, strerror(errno));
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
        if (errno != 0) {
            save_errno = errno;
            LOGV("Failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, strerror(errno));
            errno = save_errno;
            return -1;
        }
        memcpy(u.x, src, m);
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            LOGV("Failed to set argument %d to \"%s\" for child %i: %s", arg, src, pid, strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
}

int trace_fake_stat(pid_t pid) {
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;
    struct stat fakebuf;

    if (0 > trace_peek(pid, syscall_args[1], &addr)) {
        save_errno = errno;
        LOGV("Failed to get address of argument 1 for child %i: %s", pid, strerror(errno));
        errno = save_errno;
        return -1;
    }

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
            LOGV("Failed to set argument 1 to %p for child %i: %s", (void *)fakeptr, pid, strerror(errno));
            errno = save_errno;
            return -1;
        }
        ++n;
        ++fakeptr;
    }

    m = sizeof(struct stat) % sizeof(long);
    if (0 != m) {
        memcpy(u.x, fakeptr, m);
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            LOGV("Failed to set argument 1 to %p for child %i: %s", (void *)fakeptr, pid, strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
}
