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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "defs.h"

void tchild_new(struct tchild **head, pid_t pid) {
    struct tchild *newchild;

    newchild = (struct tchild *) xmalloc(sizeof(struct tchild));
    newchild->pid = pid;
    newchild->need_setup = 1;
    newchild->in_syscall = 0;
    newchild->orig_syscall = 0xbadca11;
    newchild->error_code = -1;
    newchild->next = *head; /* link next */
    *head = newchild; /* link head */
    lg(LOG_DEBUG, "children.new", "New child %i", pid);
}

void tchild_free(struct tchild **head) {
    struct tchild *current, *temp;

    lg(LOG_DEBUG, "children.free", "Freeing children %p", (void *) head);
    current = *head;
    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp);
    }
    *head = NULL;
}

void tchild_delete(struct tchild **head, pid_t pid) {
    struct tchild *temp;
    struct tchild *previous, *current;

    if (pid == (*head)->pid) { /* Deleting first node */
        temp = *head;
        *head = (*head)->next;
        free(temp);
    }
    else {
        previous = *head;
        current = (*head)->next;

        /* Find the correct location */
        while (NULL != current && pid != current->pid) {
            previous = current;
            current = current->next;
        }

        if (NULL != current) {
            temp = current;
            previous->next = current->next;
            free(temp);
        }
    }
}

struct tchild *tchild_find(struct tchild **head, pid_t pid) {
    struct tchild *current;

    current = *head;
    while (NULL != current) {
        if (pid == current->pid)
            return current;
        current = current->next;
    }
    return NULL;
}

void tchild_setup(struct tchild *child) {
    /* We want to trace all sub children and want special notify to distinguish
     * between normal sigtrap and syscall sigtrap.
     */
    lg(LOG_DEBUG, "children.setup",
            "Setting tracing options for child %i", child->pid);
    if (0 != ptrace(PTRACE_SETOPTIONS,
                    child->pid,
                    NULL,
                    PTRACE_O_TRACESYSGOOD
                    | PTRACE_O_TRACECLONE
                    | PTRACE_O_TRACEEXEC
                    | PTRACE_O_TRACEFORK
                    | PTRACE_O_TRACEVFORK))
        die(EX_SOFTWARE, "PTRACE_SETOPTIONS failed for child %i: %s",
                child->pid, strerror(errno));
    child->need_setup = 0;
}

/* Learn the cause of the signal received from child. */
unsigned int tchild_event(struct tchild *child, int status) {
    unsigned int event;
    int sig;

    if (WIFSTOPPED(status)) {
        /* Execution of child stopped by a signal */
        sig = WSTOPSIG(status);
        if (sig == SIGSTOP) {
            if (NULL != child && child->need_setup) {
                lg(LOG_DEBUG, "children.event.setup",
                        "Child %i is born and she's ready for tracing", child->pid);
                return E_SETUP;
            }
            if (NULL == child) {
                lg(LOG_DEBUG, "children.event.setup.premature",
                        "Child is born before fork event and she's ready for tracing");
                return E_SETUP_PREMATURE;
            }
        }
        else if (sig & SIGTRAP) {
            /* We got a signal from ptrace. */
            if (sig == (SIGTRAP | 0x80)) {
                /* Child made a system call */
                return E_SYSCALL;
            }
            event = (status >> 16) & 0xffff;
            if (PTRACE_EVENT_FORK == event) {
                lg(LOG_DEBUG, "children.event.fork.fork",
                            "Child %i called fork()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_VFORK == event) {
                lg(LOG_DEBUG, "children.event.fork.vfork",
                            "Child %i called vfork()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_CLONE == event) {
                lg(LOG_DEBUG, "children.event.fork.clone",
                            "Child %i called clone()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_EXEC == event) {
                lg(LOG_DEBUG, "children.event.execv",
                        "Child %i called execve()", child->pid);
                return E_EXECV;
            }
        }
        else {
            /* Genuine signal directed to child itself */
            lg(LOG_DEBUG, "children.event.genuine",
                    "Child %i received a signal", child->pid);
            return E_GENUINE;
        }
    }
    else if (WIFEXITED(status)) {
        lg(LOG_DEBUG, "children.event.exit",
                "Child %i exited normally", child->pid);
        return E_EXIT;
    }
    else if (WIFSIGNALED(status)) {
        lg(LOG_DEBUG, "children.event.exit.signal",
                "Child %i was terminated by a signal", child->pid);
        return E_EXIT_SIGNAL;
    }
    return E_UNKNOWN;
}
