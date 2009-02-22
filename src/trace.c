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
#include <string.h>
#include <sys/ptrace.h>

#include "defs.h"

int trace_peek(pid_t pid, long off, long *res) {
    long val;

    errno = 0;
    val = ptrace(PTRACE_PEEKUSER, pid, off, NULL);
    if (-1 == val && 0 != errno) {
        lg(LOG_ERROR, "util.trace_peek",
                "ptrace(PTRACE_PEEKUSER,%d,%lu,NULL) failed: %s",
                pid, off, strerror(errno));
        return -1;
    }
    *res = val;
    return 0;
}

int trace_get_syscall(pid_t pid, long *syscall) {
    if (0 > trace_peek(pid, ORIG_ACCUM, syscall)) {
        lg(LOG_ERROR, "trace.get.syscall",
                "Failed to get syscall: %s", strerror(errno));
        return -1;
    }
    return 0;
}

void ptrace_set_syscall(pid_t pid, int syscall) {
    if (0 > ptrace(PTRACE_POKEUSER, pid, ORIG_ACCUM, syscall)) {
        lg(LOG_ERROR, "trace.set_syscall.fail",
                "Failed to set syscall number to %d for child %i: %s",
                syscall, pid, strerror(errno));
        die(EX_SOFTWARE, "Failed to set syscall number: %s", strerror(errno));
    }
}

#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
void ptrace_get_string(pid_t pid, int arg, char *dest, size_t len) {
    int n, m;
    long addr;
    union {
        long val;
        char x[sizeof(long)];
    } u;

    /* Shut the compiler up */
    addr = 0;

    assert(arg >= 0 && arg < MAX_ARGS);
    if (0 > trace_peek(pid, syscall_args[arg], &addr))
        die(EX_SOFTWARE, "Failed to get address of argument %d: %s",
                arg + 1, strerror(errno));

    if (addr & (sizeof(long) -1)) {
        /* addr not a multiple of sizeof(long) */
        n = addr - (addr & -sizeof(long)); /* residue */
        addr &= -sizeof(long); /* residue */
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        memcpy(dest, &u.x[n], m = MIN(sizeof(long) - n, len));
        addr += sizeof(long), dest += m, len -= m;
    }
    while (len > 0) {
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (EIO == errno)
            break;
        memcpy(dest, u.x, m = MIN(sizeof(long), len));
        addr += sizeof(long), dest += m, len -= m;
    }
}

void ptrace_set_string(pid_t pid, int arg, const char *src, size_t len) {
    int n, m;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;

    assert(arg >= 0 && arg < MAX_ARGS);
    if (0 > trace_peek(pid, syscall_args[arg], &addr))
        die(EX_SOFTWARE, "Failed to get address of argument %d: %s",
                arg + 1, strerror(errno));

    n = 0;
    m = len / sizeof(long);

    while (n < m) {
        memcpy(u.x, src, sizeof(long));
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            lg(LOG_ERROR, "trace.set_string.fail_pokedata",
                    "Failed to set argument %d of the last syscall made by child %i to \"%s\": %s",
                    arg + 1, pid, src, strerror(errno));
            die(EX_SOFTWARE, "Failed to set string");
        }
        ++n;
        src += sizeof(long);
    }

    m = len % sizeof(long);
    if (0 != m) {
        memcpy(u.x, src, m);
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            lg(LOG_ERROR, "trace.set_string.fail_pokedata",
                    "Failed to set argument %d of the last syscall made by child %i to \"%s\": %s",
                    arg + 1, pid, src, strerror(errno));
            die(EX_SOFTWARE, "Failed to set string");
        }
    }
}
