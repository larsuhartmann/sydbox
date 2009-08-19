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

#ifndef SYDBOX_GUARD_SYDBOX_H
#define SYDBOX_GUARD_SYDBOX_H 1

#include <glib.h>

#include <sydbox/unstable.h>
#include <sydbox/context.h>

G_BEGIN_DECLS

/**
 * execl() wrapper.
 */
gint sydbox_execl(const struct sydbox_context * const ctx, const gchar *path,
        const gchar *arg, ...);

/**
 * execlp() wrapper.
 */
gint sydbox_execlp (const struct sydbox_context * const ctx, const gchar *file,
        const gchar *arg, ...);

/**
 * execle() wrapper.
 */
gint sydbox_execle(const struct sydbox_context * const ctx, const gchar *path,
        const gchar *arg, ..., gchar * const envp[]);

/**
 * execv() wrapper.
 */
gint sydbox_execv(const struct sydbox_context * const ctx, const gchar *path,
        gchar * const argv[]);

/**
 * execvp() wrapper.
 */
gint sydbox_execvp(const struct sydbox_context * const ctx, const gchar *file,
        gchar * const argv[]);

/**
 * execvp() wrapper
 */
gint sydbox_execve(const struct sydbox_context * const ctx, const gchar *filename,
        gchar * const argv[], gchar * const envp[]);

G_END_DECLS

#endif // SYDBOX_GUARD_SYDBOX_H

