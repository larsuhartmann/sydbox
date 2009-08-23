/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#include <stdbool.h>
#include <sys/types.h>

#include <glib.h>

#include <sydbox/context.h>

struct sydbox_context
{
    // Public
    bool sandbox_path;
    bool sandbox_execve;
    bool sandbox_network;

    GSList *write_prefixes;
    GSList *execve_prefixes;

    // Internal
    pid_t eldest;                   // first child's pid is kept to determine return code.
    bool before_initial_execve;     // first execve() is noted here for execve(2) sandboxing.
    GSList *children;               // list of children.
};

struct sydbox_context *sydbox_context_new(void)
{
    struct sydbox_context *ctx = g_try_new0(struct sydbox_context, 1);
    if (NULL == ctx)
        return NULL;
    ctx->before_initial_execve = true;
    return ctx;
}

void sydbox_context_free(struct sydbox_context *ctx)
{
    g_free(ctx);
}

bool sydbox_context_get_sandbox_path (const struct sydbox_context * const ctx)
{
    return ctx->sandbox_path;
}

void sydbox_context_set_sandbox_path(struct sydbox_context * const ctx, bool enabled)
{
    ctx->sandbox_path = enabled;
}

bool sydbox_context_get_sandbox_execve(const struct sydbox_context * const ctx)
{
    return ctx->sandbox_execve;
}

void sydbox_context_set_sandbox_execve(struct sydbox_context * const ctx, bool enabled)
{
    ctx->sandbox_execve = enabled;
}

bool sydbox_context_get_sandbox_network(const struct sydbox_context * const ctx)
{
    return ctx->sandbox_network;
}

void sydbox_context_set_sandbox_network(struct sydbox_context * const ctx, bool enabled)
{
    ctx->sandbox_network = enabled;
}

const GSList *sydbox_context_get_write_prefixes(const struct sydbox_context * const ctx)
{
    return ctx->write_prefixes;
}

void sydbox_context_set_write_prefixes(struct sydbox_context * const ctx,
        const GSList * const prefixes)
{
    gchar *prefix;
    const GSList *entry;

    if (ctx->write_prefixes)
        g_slist_free(ctx->write_prefixes);

    for (entry = prefixes; NULL != entry; entry = g_slist_next(entry)) {
        prefix = (gchar *) entry->data;
        g_assert(NULL != prefix);
        g_assert(g_path_is_absolute(prefix));
        ctx->write_prefixes = g_slist_append(ctx->write_prefixes, g_strdup(prefix));
    }
}

const GSList *sydbox_context_get_execve_prefixes(const struct sydbox_context * const ctx)
{
    return ctx->execve_prefixes;
}

void sydbox_context_set_execve_prefixes(struct sydbox_context * const ctx,
        const GSList * const prefixes)
{
    gchar *prefix;
    const GSList *entry;

    if (ctx->execve_prefixes)
        g_slist_free (ctx->execve_prefixes);

    for (entry = prefixes; NULL != entry; entry = g_slist_next(entry)) {
        prefix = (gchar *) entry->data;
        g_assert(NULL != prefix);
        g_assert(g_path_is_absolute(prefix));
        ctx->execve_prefixes = g_slist_append(ctx->execve_prefixes, g_strdup(prefix));
    }
}

