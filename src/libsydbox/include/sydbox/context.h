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

#ifndef SYDBOX_GUARD_CONTEXT_H
#define SYDBOX_GUARD_CONTEXT_H 1

#include <stdbool.h>

#include <glib.h>

#include <sydbox/unstable.h>

G_BEGIN_DECLS

/**
 * This opaque object represents a sydbox context.
 * Create a new instance with sydbox_context_new.
 */
struct sydbox_context;

/**
 * Create a new sydbox context instance.
 * Returns NULL if memory allocation fails and sets errno accordingly.
 */
struct sydbox_context *sydbox_context_new(void);

/**
 * Frees the given sydbox context.
 */
void sydbox_context_free(struct sydbox_context *ctx);

/**
 * Returns a boolean that specifies whether path sandboxing is enabled.
 */
bool sydbox_context_get_sandbox_path(const struct sydbox_context * const ctx);

/**
 * Sets path sandboxing on/off.
 */
void sydbox_context_set_sandbox_path(struct sydbox_context * const ctx, bool enabled);

/**
 * Returns a boolean that specifies whether execve(2) sandboxing is enabled.
 */
bool sydbox_context_get_sandbox_execve(const struct sydbox_context * const ctx);

/**
 * Sets execve(2) sandboxing on/off.
 */
void sydbox_context_set_sandbox_execve(struct sydbox_context * const ctx, bool enabled);

/**
 * Returns a boolean that specifies whether network sandboxing is enabled.
 */
bool sydbox_context_get_sandbox_network(const struct sydbox_context * const ctx);

/**
 * Sets network sandboxing on/off.
 */
void sydbox_context_set_sandbox_network(struct sydbox_context * const ctx, bool enabled);

/**
 * Returns a list of write allowed path prefixes.
 */
const GSList *sydbox_context_get_write_prefixes(const struct sydbox_context * const ctx);

/**
 * Sets the list of write allowed path prefixes.
 */
void sydbox_context_set_write_prefixes(struct sydbox_context * const ctx,
        const GSList * const prefixes);

/**
 * Returns a list of execve(2) allowed path prefixes.
 */
const GSList *sydbox_context_get_execve_prefixes(const struct sydbox_context * const ctx);

/**
 * Sets the list of execve(2) allowed path prefixes.
 */
void sydbox_context_set_execve_prefixes(struct sydbox_context * const ctx,
        const GSList * const prefixes);

G_END_DECLS

#endif // SYDBOX_GUARD_CONTEXT_H

