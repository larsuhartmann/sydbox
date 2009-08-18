/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
 * Based in part upon catbox which is:
 *  Copyright (c) 2006-2007 TUBITAK/UEKAE
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#include "dispatch.h"
#include "loop.h"
#include "proc.h"
#include "trace.h"
#include "syscall.h"
#include "children.h"
#include "sydbox-config.h"
#include "sydbox-log.h"


// Event handlers
static int xsetup(context_t *ctx, struct tchild *child) {
    if (0 > trace_setup(child->pid)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to set tracing options: %s", g_strerror(errno));
            g_printerr("failed to set tracing options: %s", g_strerror (errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    else
        child->flags &= ~TCHILD_NEEDSETUP;
    return 0;
}

static int xsyscall(context_t *ctx, struct tchild *child) {
    if (0 > trace_syscall(child->pid, 0)) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i: %s", child->pid, g_strerror (errno));
            g_printerr("failed to resume child %i: %s", child->pid, g_strerror (errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    return 0;
}

static int xfork(context_t *ctx, struct tchild *child) {
    pid_t childpid;
    struct tchild *newchild;

    // Get new child's pid
    if (G_UNLIKELY(0 > trace_geteventmsg(child->pid, &childpid))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to get the pid of the newborn child: %s", g_strerror(errno));
            g_printerr("failed to get the pid of the newborn child: %s", g_strerror (errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    else {
#if defined(POWERPC)
        /* Special case for POWERPC.
         * GETEVENTMSG doesn't work reliably on this architecture as it seems.
         * Instead we use a hack in syscall.c
         */
        if (childpid == 0) {
            g_debug("PTRACE_GETEVENTMSG returned 0 as pid, ignoring!");
            return 0;
        }
#endif // POWERPC
        g_debug("the newborn child's pid is %i", childpid);
    }

    newchild = tchild_find(ctx->children, childpid);
    if (NULL == newchild) {
        /* Child hasn't been born yet, add it to the list of children and
         * inherit parent's sandbox data.
         */
        tchild_new(&(ctx->children), childpid);
        newchild = tchild_find(ctx->children, childpid);
        tchild_inherit(newchild, child);
    }
    else if (!newchild->inherited) {
        /* Child has already been born but hasn't inherited parent's sandbox data
         * yet. Inherit parent's sandbox data and resume the child.
         */
        g_debug("prematurely born child %i inherits sandbox data from her parent %i", newchild->pid, child->pid);
        tchild_inherit(newchild, child);
        xsyscall(ctx, newchild);
    }
    return 0;
}

static int xgenuine(context_t * ctx, struct tchild *child, int status) {
    if (G_UNLIKELY(0 > trace_syscall(child->pid, WSTOPSIG(status)))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i after genuine signal: %s", child->pid, g_strerror(errno));
            g_printerr("failed to resume child %i after genuine signal: %s", child->pid, g_strerror(errno));
            exit(-1);
        }
        return context_remove_child(ctx, child->pid);
    }
    g_debug("resumed child %i after genuine signal", child->pid);
    return 0;
}

static int xunknown(context_t *ctx, struct tchild *child, int status) {
    if (G_UNLIKELY(0 > trace_syscall(child->pid, WSTOPSIG(status)))) {
        if (G_UNLIKELY(ESRCH != errno)) {
            g_critical("failed to resume child %i after unknown signal %#x: %s",
                    child->pid, WSTOPSIG(status), g_strerror(errno));
            g_printerr("failed to resume child %i after unknown signal %#x: %s",
                    child->pid, WSTOPSIG(status), g_strerror(errno));
            exit(-1);
        }
        return context_remove_child (ctx, child->pid);
    }
    g_debug("resumed child %i after unknown signal %#x", child->pid, status);
    return 0;
}

int trace_loop(context_t *ctx) {
    int status, ret;
    unsigned int event;
    pid_t pid;
    struct tchild *child;

    ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL);
        if (G_UNLIKELY(0 > pid)) {
            if (ECHILD == errno)
                break;
            else {
                g_critical ("waitpid failed: %s", g_strerror (errno));
                g_printerr ("waitpid failed: %s", g_strerror (errno));
                exit (-1);
            }
        }
        child = tchild_find(ctx->children, pid);
        event = trace_event(status);
        assert(NULL != child || E_STOP == event || E_EXIT == event || E_EXIT_SIGNAL == event);

        switch(event) {
            case E_STOP:
                g_debug("latest event for child %i is E_STOP, calling event handler", pid);
                if (NULL == child) {
                    /* Child is born before PTRACE_EVENT_FORK.
                     * Set her up but don't resume her until we receive the
                     * event.
                     */
                    g_debug("setting up prematurely born child %i", pid);
                    tchild_new(&(ctx->children), pid);
                    child = tchild_find(ctx->children, pid);
                    ret = xsetup(ctx, child);
                    if (0 != ret)
                        return ret;
                }
                else {
                    g_debug("setting up child %i", child->pid);
                    ret = xsetup(ctx, child);
                    if (0 != ret)
                        return ret;
                    ret = xsyscall(ctx, child);
                    if (0 != ret)
                        return ret;
                }
                break;
            case E_SYSCALL:
                ret = syscall_handle(ctx, child);
                if (0 != ret)
                    return ret;
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return ret;
                break;
            case E_FORK:
            case E_VFORK:
            case E_CLONE:
                g_debug ("latest event for child %i is E_FORK, calling event handler", pid);
                ret = xfork(ctx, child);
                if (0 != ret)
                    return ret;
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return ret;
                break;
            case E_EXEC:
                g_debug ("latest event for child %i is E_EXEC, calling event handler", pid);
                // Check for exec_lock
                if (G_UNLIKELY(LOCK_PENDING == child->sandbox->lock)) {
                    g_info("access to magic commands is now denied for child %i", child->pid);
                    child->sandbox->lock = LOCK_SET;
                }
#if defined(X86_64)
                // Update child's personality
                child->personality = trace_type(child->pid);
                if (0 > child->personality) {
                    g_critical("failed to determine personality of child %i: %s", child->pid, g_strerror(errno));
                    g_printerr("failed to determine personality of child %i: %s", child->pid, g_strerror(errno));
                    exit(-1);
                }
                g_debug("updated child %i's personality to %s mode", child->pid, dispatch_mode(child->personality));
#endif // defined(X86_64)
                ret = xsyscall(ctx, child);
                if (0 != ret)
                    return ret;
                break;
            case E_GENUINE:
                g_debug ("latest event for child %i is E_GENUINE, calling event handler", pid);
                ret = xgenuine(ctx, child, status);
                if (0 != ret)
                    return ret;
                break;
            case E_EXIT:
                if (G_UNLIKELY(ctx->eldest == pid)) {
                    // Eldest child, keep return value.
                    ret = WEXITSTATUS(status);
                    if (0 != ret)
                        g_message("eldest child %i exited with return code %d", pid, ret);
                    else
                        g_info("eldest child %i exited with return code %d", pid, ret);
                    if (!sydbox_config_get_wait_all())
                        return ret;
                }
                else
                    g_debug("child %i exited with return code: %d", pid, ret);
                tchild_delete(&(ctx->children), pid);
                break;
            case E_EXIT_SIGNAL:
                if (G_UNLIKELY(ctx->eldest == pid)) {
                    ret = EXIT_FAILURE;
                    g_message("eldest child %i exited with signal %d", pid, WTERMSIG(status));
                    if (!sydbox_config_get_wait_all())
                        return ret;
                }
                else
                    g_info("child %i exited with signal %d", pid, WTERMSIG(status));
                tchild_delete(&(ctx->children), pid);
                break;
            case E_UNKNOWN:
                g_info("unknown signal %#x received from child %i", WSTOPSIG(status), pid);
                ret = xunknown(ctx, child, status);
                if (0 != ret)
                    return ret;
                break;
        }
    }
    return ret;
}

