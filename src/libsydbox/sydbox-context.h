/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef __SYDBOX_CONTEXT_H__
#define __SYDBOX_CONTEXT_H__

#include <glib.h>

#include <sydbox-unstable.h>

G_BEGIN_DECLS

typedef struct _SydboxContext SydboxContext;

SydboxContext *
sydbox_context_create (void);

void
sydbox_context_destroy (SydboxContext *ctx);

gboolean
sydbox_context_get_sandbox_network (const SydboxContext * const ctx);

void
sydbox_context_set_sandbox_network (SydboxContext * const ctx);

const GSList *
sydbox_context_get_write_prefixes (const SydboxContext * const ctx);

void
sydbox_context_set_write_prefixes (SydboxContext * const ctx,
                                   const GSList * const prefixes);

const GSList *
sydbox_context_get_predict_prefixes (const SydboxContext * const ctx);

void
sydbox_context_set_predict_prefixes (SydboxContext * const ctx,
                                     const GSList * const prefixes);

G_END_DECLS

#endif

