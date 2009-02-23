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

#define ADDR_MUL        ((64 == __WORDSIZE) ? 8 : 4)
#define MAX_ARGS        4
#if defined(I386)
#define ORIG_ACCUM      (4 * ORIG_EAX)
#define ACCUM           (4 * EAX)
static const long syscall_args[MAX_ARGS] = {4 * EBX, 4 * ECX, 4 * EDX, 4 * ESI};
#elif defined(X86_64)
#define ORIG_ACCUM      (8 * ORIG_RAX)
#define ACCUM           (8 * RAX)
static const long syscall_args[MAX_ARGS] = {8 * RDI, 8 * RSI, 8 * RDX, 8 * R10};
#endif

int trace_peek(pid_t pid, long off, long *res) {
    long val;

    errno = 0;
    val = ptrace(PTRACE_PEEKUSER, pid, off, NULL);
    if (-1 == val && 0 != errno) {
        int save_errno = errno;
        lg(LOG_ERROR, "trace.peek",
                "ptrace(PTRACE_PEEKUSER,%d,%lu,NULL) failed: %s",
                pid, off, strerror(errno));
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
        lg(LOG_ERROR, "trace.get.arg",
                "Failed to get argument %d: %s", arg, strerror(errno));
        errno = save_errno;
        return -1;
    }
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

int trace_set_syscall(pid_t pid, long syscall) {
    if (0 > ptrace(PTRACE_POKEUSER, pid, ORIG_ACCUM, syscall)) {
        int save_errno = errno;
        lg(LOG_ERROR, "trace.set.syscall",
                "Failed to set syscall number to %ld for child %i: %s",
                syscall, pid, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_set_return(pid_t pid, long val) {
    if (0 != ptrace(PTRACE_POKEUSER, pid, ACCUM, val)) {
        int save_errno = errno;
        lg(LOG_ERROR, "trace.set.return",
                "ptrace(PTRACE_POKEUSER,%i,ACCUM,%ld) failed: %s",
                pid, val, strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
int trace_get_string(pid_t pid, int arg, char *dest, size_t len) {
    int n, m, save_errno;
    long addr = 0;
    union {
        long val;
        char x[sizeof(long)];
    } u;

    assert(arg >= 0 && arg < MAX_ARGS);
    if (0 > trace_peek(pid, syscall_args[arg], &addr)) {
        save_errno = errno;
        lg(LOG_ERROR, "trace.set.string",
                "Failed to get address of argument %d: %s",
                arg, strerror(errno));
        errno = save_errno;
        return -1;
    }

    if (addr & (sizeof(long) -1)) {
        /* addr not a multiple of sizeof(long) */
        n = addr - (addr & -sizeof(long)); /* residue */
        addr &= -sizeof(long); /* residue */
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (-1 == u.val && 0 != errno) {
            save_errno = errno;
            lg(LOG_ERROR, "trace.set.string",
                    "ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s",
                    pid, addr, strerror(errno));
            errno = save_errno;
            return -1;
        }
        memcpy(dest, &u.x[n], m = MIN(sizeof(long) - n, len));
        addr += sizeof(long), dest += m, len -= m;
    }
    while (len > 0) {
        u.val = ptrace(PTRACE_PEEKDATA, pid, (char *) addr, NULL);
        if (-1 == u.val) {
            if (EIO == errno)
                break;
            else if (0 != errno) {
                save_errno = errno;
                lg(LOG_ERROR, "trace.set.string.rest",
                        "ptrace(PTRACE_PEEKDATA,%i,%ld,NULL) failed: %s",
                        pid, addr, strerror(errno));
                errno = save_errno;
                return -1;
            }
        }
        memcpy(dest, u.x, m = MIN(sizeof(long), len));
        addr += sizeof(long), dest += m, len -= m;
    }
    return 0;
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
        lg(LOG_ERROR, "trace.set.string",
                "Failed to get address of argument %d: %s", arg, strerror(errno));
        errno = save_errno;
        return -1;
    }

    n = 0;
    m = len / sizeof(long);

    while (n < m) {
        memcpy(u.x, src, sizeof(long));
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            lg(LOG_ERROR, "trace.set.string.pokedata",
                    "Failed to set argument %d to \"%s\": %s",
                    arg, src, strerror(errno));
            errno = save_errno;
            return -1;
        }
        ++n;
        src += sizeof(long);
    }

    m = len % sizeof(long);
    if (0 != m) {
        memcpy(u.x, src, m);
        if (0 > ptrace(PTRACE_POKEDATA, pid, addr + n * ADDR_MUL, u.val)) {
            save_errno = errno;
            lg(LOG_ERROR, "trace.set.string.pokedata",
                    "Failed to set argument %d to \"%s\": %s",
                    arg, src, strerror(errno));
            errno = save_errno;
            return -1;
        }
    }
    return 0;
}

void trace_request_name(int request, char *dest) {
    switch (request) {
        case PTRACE_PEEKTEXT:
            strcpy(dest, "PTRACE_PEEKTEXT");
            break;
        case PTRACE_PEEKDATA:
            strcpy(dest, "PTRACE_PEEKDATA");
            break;
        case PTRACE_PEEKUSER:
            strcpy(dest, "PTRACE_PEEKUSER");
            break;
        case PTRACE_POKETEXT:
            strcpy(dest, "PTRACE_POKETEXT");
            break;
        case PTRACE_POKEUSER:
            strcpy(dest, "PTRACE_POKEDATA");
            break;
        case PTRACE_CONT:
            strcpy(dest, "PTRACE_PTRACECONT");
            break;
        case PTRACE_SINGLESTEP:
            strcpy(dest, "PTRACE_SINGLESTEP");
            break;
        case PTRACE_GETREGS:
            strcpy(dest, "PTRACE_GETREGS");
            break;
        case PTRACE_SETREGS:
            strcpy(dest, "PTRACE_SETREGS");
            break;
        case PTRACE_GETFPREGS:
            strcpy(dest, "PTRACE_GETFPREGS");
            break;
        case PTRACE_SETFPREGS:
            strcpy(dest, "PTRACE_SETFPREGS");
            break;
        case PTRACE_GETFPXREGS:
            strcpy(dest, "PTRACE_GETFPXREGS");
            break;
        case PTRACE_SETFPXREGS:
            strcpy(dest, "PTRACE_SETFPXREGS");
            break;
        case PTRACE_SYSCALL:
            strcpy(dest, "PTRACE_SYSCALL");
            break;
        case PTRACE_GETEVENTMSG:
            strcpy(dest, "PTRACE_GETEVENTMSG");
            break;
        case PTRACE_GETSIGINFO:
            strcpy(dest, "PTRACE_GETSIGINFO");
            break;
        case PTRACE_SETSIGINFO:
            strcpy(dest, "PTRACE_SETSIGINFO");
            break;
        default:
            strcpy(dest, "UNKNOWN");
            break;
    }
}

int trace_emulate(struct tchild *child) {
    int request;
    pid_t pid;
    void *addr, *data;
    long ret;

    if (0 > trace_get_arg(child->pid, 0, (long *) &request)) {
        lg(LOG_ERROR, "trace.emulate",
                "Failed to get request argument of ptrace: %s", strerror(errno));
        return -1;
    }
    else if (0 > trace_get_arg(child->pid, 1, (long *) &pid)) {
        lg(LOG_ERROR, "trace.emulate",
                "Failed to get pid argument of ptrace: %s", strerror(errno));
        return -1;
    }
    else if (0 > trace_get_arg(child->pid, 2, (long *) &addr)) {
        lg(LOG_ERROR, "trace.emulate",
                "Failed to get addr argument of ptrace: %s", strerror(errno));
        return -1;
    }
    else if (0 > trace_get_arg(child->pid, 3, (long *) &data)) {
        lg(LOG_ERROR, "trace.emulate",
                "Failed to get data argument of ptrace: %s", strerror(errno));
        return -1;
    }

    if (PTRACE_TRACEME == request) {
        lg(LOG_DEBUG, "trace.emulate",
                "PTRACE_TRACEME requested, no need to call ptrace()");
        child->retval = 0;
        return 0;
    }
    else if (PTRACE_ATTACH == request) {
        lg(LOG_DEBUG, "trace.emulate",
                "PTRACE_ATTACH requested, no need to call ptrace()");
        child->retval = 0;
        return 0;
    }
    else if (PTRACE_DETACH == request) {
        lg(LOG_DEBUG, "trace.emulate",
                "PTRACE_DETACH requested, refusing to detach");
        child->retval = 0;
        return 0;
    }
    else if (PTRACE_SETOPTIONS == request) {
        lg(LOG_DEBUG, "trace.emulate",
                "PTRACE_SETOPTIONS requested, refusing to set options");
        child->retval = 0;
        return 0;
    }

    if (LOG_DEBUG <= log_level) {
        char rname[24];
        trace_request_name(request, rname);
        lg(LOG_DEBUG, "trace.emulate",
                "Calling ptrace(%s,%i,%p,%p", rname, pid, addr, data);
    }

    ret = ptrace(request, pid, addr, data);
    if (0 != errno)
        child->retval = -errno;
    else
        child->retval = ret;

    return 0;
}
