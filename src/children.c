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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "path.h"
#include "children.h"
#include "sydbox-log.h"
#include "sydbox-config.h"

void tchild_new(GSList **children, pid_t pid) {
    gchar *proc_pid;
    struct tchild *child;

    g_debug("new child %i", pid);
    child = (struct tchild *) g_malloc (sizeof(struct tchild));
    child->flags = TCHILD_NEEDSETUP;
    child->pid = pid;
    child->sno = 0xbadca11;
    child->retval = -1;
    child->cwd = NULL;
    child->inherited = false;
    child->sandbox = (struct tdata *) g_malloc (sizeof(struct tdata));
    child->sandbox->path = true;
    child->sandbox->exec = false;
    child->sandbox->network = false;
    child->sandbox->lock = LOCK_UNSET;
    child->sandbox->write_prefixes = NULL;
    child->sandbox->predict_prefixes = NULL;
    child->sandbox->exec_prefixes = NULL;

    if (sydbox_config_get_allow_proc_pid()) {
        /* Allow /proc/%d which is needed for processes to work reliably.
         * FIXME: This path will be inherited by children as well.
         */
        proc_pid = g_strdup_printf("/proc/%i", pid);
        pathnode_new(&(child->sandbox->write_prefixes), proc_pid, 0);
        g_free(proc_pid);
    }

    *children = g_slist_prepend(*children, child);
}

void tchild_inherit(struct tchild *child, struct tchild *parent)
{
    GSList *walk;

    g_assert(NULL != child && NULL != parent);
    if (child->inherited)
        return;

    if (G_LIKELY(NULL != parent->cwd)) {
        g_debug("child %i inherits parent %i's current working directory `%s'", child->pid, parent->pid, parent->cwd);
        child->cwd = g_strdup(parent->cwd);
    }

    child->personality = parent->personality;
    child->sandbox->path = parent->sandbox->path;
    child->sandbox->exec = parent->sandbox->exec;
    child->sandbox->network = parent->sandbox->network;
    child->sandbox->lock = parent->sandbox->lock;
    // Copy path lists
    walk = parent->sandbox->write_prefixes;
    while (NULL != walk) {
        pathnode_new(&(child->sandbox->write_prefixes), walk->data, 0);
        walk = g_slist_next(walk);
    }
    walk = parent->sandbox->predict_prefixes;
    while (NULL != walk) {
        pathnode_new(&(child->sandbox->predict_prefixes), walk->data, 0);
        walk = g_slist_next(walk);
    }
    walk = parent->sandbox->exec_prefixes;
    while (NULL != walk) {
        pathnode_new(&(child->sandbox->exec_prefixes), walk->data, 0);
        walk = g_slist_next(walk);
    }

    child->inherited = true;
}

static void tchild_free_one(struct tchild *child, void *user_data G_GNUC_UNUSED) {
    if (G_LIKELY(NULL != child->sandbox)) {
        if (G_LIKELY(NULL != child->sandbox->write_prefixes))
            pathnode_free(&(child->sandbox->write_prefixes));
        if (G_LIKELY(NULL != child->sandbox->predict_prefixes))
            pathnode_free(&(child->sandbox->predict_prefixes));
        if (G_LIKELY(NULL != child->sandbox->exec_prefixes))
            pathnode_free(&(child->sandbox->exec_prefixes));
        g_free (child->sandbox);
    }
    if (G_LIKELY(NULL != child->cwd))
        g_free (child->cwd);
    g_free (child);
}

void tchild_free(GSList **children) {
    g_debug ("freeing children %p", (void *) *children);
    g_slist_foreach(*children, (GFunc) tchild_free_one, NULL);
    g_slist_free(*children);
    *children = NULL;
}

void tchild_delete(GSList **children, pid_t pid) {
    GSList *walk;
    struct tchild *child;

    walk = *children;
    while (NULL != walk) {
        child = (struct tchild *) walk->data;
        if (child->pid == pid) {
            *children = g_slist_remove_link(*children, walk);
            tchild_free_one(walk->data, NULL);
            g_slist_free(walk);
            break;
        }
        walk = g_slist_next(walk);
    }
}

struct tchild *tchild_find(GSList *children, pid_t pid)
{
    GSList *walk;
    struct tchild *child;

    walk = children;
    while (NULL != walk) {
        child = (struct tchild *) walk->data;
        if (pid == child->pid)
            return child;
        walk = g_slist_next(walk);
    }
    return NULL;
}

