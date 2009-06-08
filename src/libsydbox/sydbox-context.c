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

#include "sydbox-context.h"

struct _SydboxContext
{
   gboolean sandbox_network;

   GSList *write_prefixes;
   GSList *predict_prefixes;
};

SydboxContext *
sydbox_context_create (void)
{
   return g_try_new0 (SydboxContext, 1);
}

void
sydbox_context_destroy (SydboxContext *ctx)
{
   g_free (ctx);
}

gboolean
sydbox_context_get_sandbox_network (const SydboxContext * const ctx)
{
   return ctx->sandbox_network;
}

void
sydbox_context_set_sandbox_network (SydboxContext * const ctx, gboolean enabled)
{
   ctx->sandbox_network = enabled;
}

const GSList *
sydbox_context_get_write_prefixes (const SydboxContext * const ctx)
{
   return ctx->write_prefixes;
}

static void
_sydbox_copy_slist (gpointer data, gpointer user_data)
{
   g_slist_append (user_data, g_strdup ((gchar *)(((GSList *) data)->data)));
}

void
sydbox_context_set_write_prefixes (SydboxContext * const ctx,
                                   const GSList * const prefixes)
{
   if (ctx->write_prefixes)
      g_slist_free (ctx->write_prefixes);
   g_slist_foreach (prefixes, _sydbox_copy_slist, ctx->write_prefixes);
}

const GSList *
sydbox_context_get_predict_prefixes (const SydboxContext * const ctx)
{
   return ctx->predict_prefixes;
}

void
sydbox_context_set_predict_prefixes (SydboxContext * const ctx,
                                     const GSList * const prefixes)
{
   if (ctx->predict_prefixes)
      g_slist_free (ctx->write_prefixes);
   g_slist_foreach (prefixes, _sydbox_copy_slist, ctx->predict_prefixes);
}

