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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include "defs.h"

void tchild_new(struct tchild **head, pid_t pid) {
    struct tchild *newchild;

    LOGD("New child %i", pid);
    newchild = (struct tchild *) xmalloc(sizeof(struct tchild));
    newchild->flags = TCHILD_NEEDSETUP;
    newchild->pid = pid;
    newchild->hasmagic = 1;
    newchild->exec_count = 1;
    newchild->syscall = 0xbadca11;
    newchild->retval = -1;
    newchild->next = *head; // link next
    if (NULL != newchild->next && NULL != newchild->next->cwd) {
        LOGD("Child %i inherits parent %i's current working directory '%s'", pid,
                newchild->next->pid, newchild->next->cwd);
        newchild->cwd = xstrndup(newchild->next->cwd, PATH_MAX);
    }
    else
        newchild->cwd = NULL;
    *head = newchild; // link head
}

void tchild_free(struct tchild **head) {
    struct tchild *current, *temp;

    LOGD("Freeing children %p", (void *) head);
    current = *head;
    while (current != NULL) {
        temp = current;
        current = current->next;
        if (NULL != temp->cwd)
            free(temp->cwd);
        free(temp);
    }
    *head = NULL;
}

void tchild_delete(struct tchild **head, pid_t pid) {
    struct tchild *temp;
    struct tchild *previous, *current;

    if (pid == (*head)->pid) { // Deleting first node
        temp = *head;
        *head = (*head)->next;
        if (NULL != temp->cwd)
            free(temp->cwd);
        free(temp);
    }
    else {
        previous = *head;
        current = (*head)->next;

        // Find the correct location
        while (NULL != current && pid != current->pid) {
            previous = current;
            current = current->next;
        }

        if (NULL != current) {
            temp = current;
            previous->next = current->next;
            if (NULL != temp->cwd)
                free(temp->cwd);
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

// Learn the cause of the signal received from child.
unsigned int tchild_event(struct tchild *child, int status) {
    unsigned int event;
    int sig;

    if (WIFSTOPPED(status)) {
        // Execution of child stopped by a signal
        sig = WSTOPSIG(status);
        if (sig == SIGSTOP) {
            if (NULL != child && child->flags & TCHILD_NEEDSETUP) {
                LOGD("Child %i is born and she's ready for tracing", child->pid);
                return E_SETUP;
            }
            if (NULL == child) {
                LOGD("Child is born before fork event and she's ready for tracing");
                return E_SETUP_PREMATURE;
            }
        }
        else if (sig & SIGTRAP) {
            // We got a signal from ptrace.
            if (sig == (SIGTRAP | 0x80)) {
                // Child made a system call
                return E_SYSCALL;
            }
            event = (status >> 16) & 0xffff;
            if (PTRACE_EVENT_FORK == event) {
                LOGD("Child %i called fork()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_VFORK == event) {
                LOGD("Child %i called vfork()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_CLONE == event) {
                LOGD("Child %i called clone()", child->pid);
                return E_FORK;
            }
            else if (PTRACE_EVENT_EXEC == event) {
                LOGD("Child %i called execve()", child->pid);
                return E_EXECV;
            }
        }
        else {
            // Genuine signal directed to child itself
            LOGD("Child %i received a signal", child->pid);
            return E_GENUINE;
        }
    }
    else if (WIFEXITED(status)) {
        LOGV("Child %i exited with return code: %d", NULL == child ? -1 : child->pid, WEXITSTATUS(status));
        return E_EXIT;
    }
    else if (WIFSIGNALED(status)) {
        LOGV("Child %i was terminated by a signal %d", NULL == child ? -1 : child->pid, WSTOPSIG(status));
        return E_EXIT_SIGNAL;
    }
    return E_UNKNOWN;
}
