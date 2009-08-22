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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#include <glib.h>

#include "context.h"
#include "wrappers.h"
#include "sydbox-log.h"

context_t *context_new (void)
{
    context_t *ctx;

    ctx = (context_t *) g_malloc0(sizeof(context_t));

    ctx->before_initial_execve = true;

    return ctx;
}

void context_free(context_t *ctx)
{
    if (G_LIKELY(NULL != ctx->children))
        tchild_free (&(ctx->children));

    g_free(ctx);
}

int context_remove_child(context_t * const ctx, pid_t pid)
{
    g_info("removing child %d from context", pid);
    tchild_delete(&ctx->children, pid);

    return (NULL == ctx->children) ? -1 : 0;
}

