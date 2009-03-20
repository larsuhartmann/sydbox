/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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
#include <time.h>

#include "defs.h"

const struct timespec waitpid_interval = { .tv_sec = 0, .tv_nsec = 32 };

// Event handlers
static int xsetup(context_t *ctx, struct tchild *child) {
    if (0 > trace_setup(child->pid)) {
        if (ESRCH == errno) // Child died
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to set tracing options: %s", strerror(errno));
    }
    else
        child->flags &= ~TCHILD_NEEDSETUP;

    if (0 > trace_syscall(child->pid, 0)) {
        if (ESRCH == errno) // Child died
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i after setup: %s", child->pid, strerror(errno));
            DIESOFT("Failed to resume child %i after setup: %s", child->pid, strerror(errno));
        }
    }
    else
        LOGC("Resumed child %i after setup", child->pid);
    return 0;
}

static int xsetup_premature(context_t *ctx, pid_t pid) {
    tchild_new(&(ctx->children), pid);
    return xsetup(ctx, ctx->children);
}

static int xsyscall(context_t *ctx, struct tchild *child) {
    if (0 > trace_syscall(child->pid, 0)) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i: %s", child->pid, strerror(errno));
            DIESOFT("Failed to resume child %i: %s", child->pid, strerror(errno));
        }
    }
    return 0;
}

static int xfork(context_t *ctx, struct tchild *child) {
    pid_t childpid;
    struct tchild *newchild;

    // Get new child's pid
    if (0 > trace_geteventmsg(child->pid, &childpid)) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to get the pid of the newborn child: %s", strerror(errno));
    }
    else
        LOGD("The newborn child's pid is %i", childpid);

    newchild = childtab[childpid];
    if (NULL != newchild) {
        LOGD("Child %i is prematurely born, letting it continue its life", newchild->pid);
        if (0 > trace_syscall(newchild->pid, 0)) {
            if (ESRCH == errno)
                return handle_esrch(ctx, newchild);
            else
                DIESOFT("Failed to resume prematurely born child %i: %s", newchild->pid, strerror(errno));
        }
        else
            LOGC("Resumed prematurely born child %i", newchild->pid);
    }
    else {
        // Add the child, setup will be done later
        tchild_new(&(ctx->children), childpid);
    }
    return xsyscall(ctx, child);
}

static int xgenuine(context_t * ctx, struct tchild *child, int status) {
    if (0 > trace_syscall(child->pid, WSTOPSIG(status))) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to resume child %i after genuine signal: %s", child->pid, strerror(errno));
    }
    else
        LOGC("Resumed child %i after genuine signal", child->pid);
    return 0;
}

static int xunknown(context_t *ctx, struct tchild *child, int status) {
    if (0 > trace_syscall(child->pid, WSTOPSIG(status))) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i after unknown signal %#x: %s", child->pid, status,
                    strerror(errno));
            DIESOFT("Failed to resume child %i after unknown signal %#x: %s", child->pid, status,
                    strerror(errno));
        }
    }
    else
        LOGC("Resumed child %i after unknown signal %#x", child->pid, status);
    return 0;
}

int trace_loop(context_t *ctx) {
    int status, ret;
    unsigned int event;
    pid_t pid;
    struct tchild *child;

    ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL | WNOHANG);
        if (likely(0 == pid)) {
            nanosleep(&waitpid_interval, NULL);
            continue;
        }
        else if (unlikely(0 > pid)) {
            LOGE("waitpid failed: %s", strerror(errno));
            DIESOFT("waitpid failed: %s", strerror(errno));
        }
        child = childtab[pid];
        event = trace_event(status);
        assert(NULL != child || E_STOP == event || E_EXIT == event || E_EXIT_SIGNAL == event);

        switch(event) {
            case E_STOP:
                LOGD("Latest event for child %i is E_STOP, calling event handler", pid);
                if (NULL == child) {
                    ret = xsetup_premature(ctx, pid);
                    if (0 != ret)
                        return ret;
                }
                else {
                    ret = xsetup(ctx, child);
                    if (0 != ret)
                        return ret;
                }
                break;
            case E_SYSCALL:
                ret = syscall_handle(ctx, child);
                if (unlikely(0 != ret))
                    return ret;
                ret = xsyscall(ctx, child);
                if (unlikely(0 != ret))
                    return ret;
                break;
            case E_FORK:
            case E_VFORK:
            case E_CLONE:
                LOGD("Latest event for child %i is E_FORK, calling event handler", pid);
                ret = xfork(ctx, child);
                if (unlikely(0 != ret))
                    return ret;
                break;
            case E_EXEC:
                LOGD("Latest event for child %i is E_EXEC, calling event handler", pid);
                ret = xsyscall(ctx, child);
                if (unlikely(0 != ret))
                    return ret;
                break;
            case E_GENUINE:
                LOGD("Latest event for child %i is E_GENUINE, calling event handler", pid);
                ret = xgenuine(ctx, child, status);
                if (unlikely(0 != ret))
                    return ret;
                break;
            case E_EXIT:
                ret = WEXITSTATUS(status);
                if (ctx->eldest == child) {
                    // Eldest child, keep the return value
                    if (0 != ret)
                        LOGN("Eldest child %i exited with return code %d", pid, ret);
                    else
                        LOGV("Eldest child %i exited with return code %d", pid, ret);
                    tchild_delete(&(ctx->children), pid);
                    return ret;
                }
                LOGD("Child %i exited with return code: %d", pid, ret);
                tchild_delete(&(ctx->children), pid);
                break;
            case E_EXIT_SIGNAL:
                if (ctx->eldest == child) {
                    LOGN("Eldest child %i exited with signal %d", pid, WTERMSIG(status));
                    tchild_delete(&(ctx->children), pid);
                    return EXIT_FAILURE;
                }
                LOGV("Child %i exited with signal %d", pid, WTERMSIG(status));
                tchild_delete(&(ctx->children), pid);
                break;
            case E_UNKNOWN:
                LOGV("Unknown signal %#x received from child %i", status, pid);
                ret = xunknown(ctx, child, status);
                if (unlikely(0 != ret))
                    return ret;
                break;
        }
    }
    return ret;
}
