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

#include <glib.h>

#include "log.h"
#include "path.h"
#include "children.h"

// We keep this for efficient lookups
struct tchild *childtab[PID_MAX_LIMIT] = { NULL };

void tchild_new(struct tchild **head, pid_t pid) {
    struct tchild *newchild;

    LOGD("New child %i", pid);
    newchild = (struct tchild *) g_malloc (sizeof(struct tchild));
    newchild->flags = TCHILD_NEEDSETUP;
    newchild->pid = pid;
    newchild->sno = 0xbadca11;
    newchild->retval = -1;
    newchild->cwd = NULL;
    newchild->sandbox = (struct tdata *) g_malloc (sizeof(struct tdata));
    newchild->sandbox->on = 1;
    newchild->sandbox->lock = LOCK_UNSET;
    newchild->sandbox->net = 1;
    newchild->sandbox->write_prefixes = NULL;
    newchild->sandbox->predict_prefixes = NULL;
    newchild->next = *head; // link next
    if (NULL != newchild->next) {
        if (NULL != newchild->next->cwd) {
            LOGD("Child %i inherits parent %i's current working directory '%s'", pid,
                    newchild->next->pid, newchild->next->cwd);
            newchild->cwd = g_strdup (newchild->next->cwd);
        }
        if (NULL != newchild->next->sandbox) {
            struct pathnode *pnode;
            newchild->sandbox->on = newchild->next->sandbox->on;
            newchild->sandbox->lock = newchild->next->sandbox->lock;
            newchild->sandbox->net = newchild->next->sandbox->net;
            // Copy path lists
            pnode = newchild->next->sandbox->write_prefixes;
            while (NULL != pnode) {
                pathnode_new(&(newchild->sandbox->write_prefixes), pnode->path, 0);
                pnode = pnode->next;
            }
            pnode = newchild->next->sandbox->predict_prefixes;
            while (NULL != pnode) {
                pathnode_new(&(newchild->sandbox->predict_prefixes), pnode->path, 0);
                pnode = pnode->next;
            }
        }
    }
    *head = newchild; // link head
    childtab[pid] = newchild;
}

static void tchild_free_one(struct tchild *child) {
    if (NULL != child->sandbox) {
        if (NULL != child->sandbox->write_prefixes)
            pathnode_free(&(child->sandbox->write_prefixes));
        if (NULL != child->sandbox->predict_prefixes)
            pathnode_free(&(child->sandbox->predict_prefixes));
        g_free (child->sandbox);
    }
    if (NULL != child->cwd)
        g_free (child->cwd);
    childtab[child->pid] = NULL;
    g_free (child);
}

void tchild_free(struct tchild **head) {
    struct tchild *current, *temp;

    LOGD("Freeing children %p", (void *) head);
    current = *head;
    while (current != NULL) {
        temp = current;
        current = current->next;
        tchild_free_one(temp);
    }
    *head = NULL;
}

void tchild_delete(struct tchild **head, pid_t pid) {
    struct tchild *temp;
    struct tchild *previous, *current;

    if (pid == (*head)->pid) { // Deleting first node
        temp = *head;
        *head = (*head)->next;
        tchild_free_one(temp);
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
            tchild_free_one(temp);
        }
    }
}
