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
#include "trace.h"
#include "sydbox-log.h"
#include "sydbox-config.h"

void tchild_new(GHashTable *children, pid_t pid)
{
    gchar *proc_pid;
    struct tchild *child;

    g_debug("new child %i", pid);
    child = (struct tchild *) g_malloc(sizeof(struct tchild));
    child->flags = TCHILD_NEEDSETUP;
    child->pid = pid;
    child->sno = 0xbadca11;
    child->retval = -1;
    child->cwd = NULL;
    child->inherited = false;
    child->sandbox = (struct tdata *) g_malloc(sizeof(struct tdata));
    child->sandbox->path = true;
    child->sandbox->exec = false;
    child->sandbox->network = false;
    child->sandbox->network_mode = SYDBOX_NETWORK_ALLOW;
    child->sandbox->network_restrict_connect = false;
    child->sandbox->lock = LOCK_UNSET;
    child->sandbox->write_prefixes = NULL;
    child->sandbox->exec_prefixes = NULL;

    if (sydbox_config_get_allow_proc_pid()) {
        /* Allow /proc/%d which is needed for processes to work reliably.
         * FIXME: This path will be inherited by children as well.
         */
        proc_pid = g_strdup_printf("/proc/%i", pid);
        pathnode_new(&(child->sandbox->write_prefixes), proc_pid, 0);
        g_free(proc_pid);
    }

    g_hash_table_insert(children, GINT_TO_POINTER(pid), child);
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
    child->sandbox->network_mode = parent->sandbox->network_mode;
    child->sandbox->network_restrict_connect = parent->sandbox->network_restrict_connect;
    child->sandbox->lock = parent->sandbox->lock;
    // Copy path lists
    walk = parent->sandbox->write_prefixes;
    while (NULL != walk) {
        pathnode_new(&(child->sandbox->write_prefixes), walk->data, 0);
        walk = g_slist_next(walk);
    }
    walk = parent->sandbox->exec_prefixes;
    while (NULL != walk) {
        pathnode_new(&(child->sandbox->exec_prefixes), walk->data, 0);
        walk = g_slist_next(walk);
    }

    child->inherited = true;
}

void tchild_free_one(gpointer child_ptr)
{
    struct tchild *child = (struct tchild *) child_ptr;

    if (G_LIKELY(NULL != child->sandbox)) {
        if (G_LIKELY(NULL != child->sandbox->write_prefixes))
            pathnode_free(&(child->sandbox->write_prefixes));
        if (G_LIKELY(NULL != child->sandbox->exec_prefixes))
            pathnode_free(&(child->sandbox->exec_prefixes));
        g_free(child->sandbox);
    }
    if (G_LIKELY(NULL != child->cwd))
        g_free(child->cwd);
    g_free(child);
}

void tchild_kill_one(gpointer pid_ptr, gpointer child_ptr G_GNUC_UNUSED, void *userdata G_GNUC_UNUSED)
{
    pid_t pid = GPOINTER_TO_INT(pid_ptr);
    trace_kill(pid);
}

void tchild_cont_one(gpointer pid_ptr, gpointer child_ptr G_GNUC_UNUSED, void *userdata G_GNUC_UNUSED)
{
    pid_t pid = GPOINTER_TO_INT(pid_ptr);
    trace_cont(pid);
}

void tchild_delete(GHashTable *children, pid_t pid)
{
    g_hash_table_remove(children, GINT_TO_POINTER(pid));
}

struct tchild *tchild_find(GHashTable *children, pid_t pid)
{
    return g_hash_table_lookup(children, GINT_TO_POINTER(pid));
}

