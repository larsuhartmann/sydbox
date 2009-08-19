/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>

#include <glib.h>

#include "sydbox-log.h"
#include "trace-util.h"

int upeek(pid_t pid, long off, long *res)
{
    long val;

    errno = 0;
    val = ptrace(PTRACE_PEEKUSER, pid, off, NULL);
    if (G_UNLIKELY(-1 == val && 0 != errno)) {
        int save_errno = errno;
        g_info("ptrace(PTRACE_PEEKUSER,%d,%lu,NULL) failed: %s", pid, off, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    *res = val;
    return 0;
}

#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
int umoven(pid_t pid, long addr, char *dest, size_t len)
{
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
        if (G_UNLIKELY(0 != errno)) {
            if (G_LIKELY(started && (EPERM == errno || EIO == errno))) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (G_UNLIKELY(0 != addr && EIO != errno)) {
                save_errno = errno;
                g_info ("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, g_strerror (errno));
                errno = save_errno;
            }
            return -1;
        }
        started = 1;
        memcpy(dest, &u.x[n], m = MIN(sizeof(long) - n, len));
        addr += sizeof(long), dest += m, len -= m;
    }
    while (len > 0) {
        errno = 0;
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (G_UNLIKELY(0 != errno)) {
            if (G_LIKELY(started && (EPERM == errno || EIO == errno))) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (G_UNLIKELY(0 != addr && EIO != errno)) {
                save_errno = errno;
                g_info ("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, g_strerror (errno));
                errno = save_errno;
            }
            return -1;
        }
        started = 1;
        memcpy(dest, u.x, m = MIN(sizeof(long), len));
        addr += sizeof(long), dest += m, len -= m;
    }
    return 0;
}

int umovestr(pid_t pid, long addr, char *dest, size_t len)
{
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
        if (G_UNLIKELY(0 != errno)) {
            if (G_LIKELY(started && (EPERM == errno || EIO == errno))) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (G_UNLIKELY(0 != addr && EIO != errno)) {
                save_errno = errno;
                g_info("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, g_strerror(errno));
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
        if (G_UNLIKELY(0 != errno)) {
            if (G_LIKELY(started && (EPERM == errno || EIO == errno))) {
                // Ran into end of memory - stupid "printpath"
                return 0;
            }
            // But if not started, we had a bogus address
            if (G_UNLIKELY(0 != addr && EIO != errno)) {
                save_errno = errno;
                g_info("ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s", pid, addr, g_strerror(errno));
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

