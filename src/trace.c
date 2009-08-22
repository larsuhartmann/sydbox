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

#include "trace.h"

/* Common functions are defined here for convenience.
 */

unsigned int trace_event(int status)
{
    int sig;
    unsigned int event;

    if (WIFSTOPPED(status)) {
        sig = WSTOPSIG(status);
        if (SIGSTOP == sig)
            return E_STOP;
        else if ((SIGTRAP | 0x80) == sig)
            return E_SYSCALL;

        event = (status >> 16) & 0xffff;
        switch (event) {
            case PTRACE_EVENT_FORK:
                return E_FORK;
            case PTRACE_EVENT_VFORK:
                return E_VFORK;
            case PTRACE_EVENT_CLONE:
                return E_CLONE;
            case PTRACE_EVENT_EXEC:
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

int trace_me(void)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_TRACEME, 0, NULL, NULL))) {
        save_errno = errno;
        g_info("failed to set tracing: %s", g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_setup(pid_t pid)
{
    int save_errno;

    g_debug("setting tracing options for child %i", pid);
    if (G_UNLIKELY(0 > ptrace(PTRACE_SETOPTIONS, pid, NULL,
                    PTRACE_O_TRACESYSGOOD
                    | PTRACE_O_TRACECLONE
                    | PTRACE_O_TRACEFORK
                    | PTRACE_O_TRACEVFORK
                    | PTRACE_O_TRACEEXEC))) {
        save_errno = errno;
        g_info("setting tracing options failed for child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_cont(pid_t pid)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_CONT, pid, NULL, NULL))) {
        save_errno = errno;
        g_info("failed to continue child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_kill(pid_t pid)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_KILL, pid, NULL, NULL) && ESRCH != errno)) {
        save_errno = errno;
        g_info("failed to kill child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

int trace_syscall(pid_t pid, int data)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_SYSCALL, pid, NULL, data))) {
        save_errno = errno;
        g_info("failed to resume child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }

    return 0;
}

int trace_geteventmsg(pid_t pid, void *data)
{
    int save_errno;

    if (G_UNLIKELY(0 > ptrace(PTRACE_GETEVENTMSG, pid, NULL, data))) {
        save_errno = errno;
        g_info("failed to get event message of child %i: %s", pid, g_strerror(errno));
        errno = save_errno;
        return -1;
    }
    return 0;
}

