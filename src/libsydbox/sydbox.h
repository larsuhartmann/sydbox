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

#ifndef __SYDBOX_H__
#define __SYDBOX_H__

#include <glib.h>

#include <sydbox-unstable.h>
#include <sydbox-context.h>

G_BEGIN_DECLS

gint
sydbox_execl (const SydboxContext * const ctx,
              const gchar * const path,
              const gchar * const arg,
              ...);

gint
sydbox_execlp (const SydboxContext * const ctx,
               const gchar * const file,
               const gchar * const arg,
               ...);

gint
sydbox_execle (const SydboxContext * const ctx,
               const gchar * const path,
               const gchar * const arg,
               ...,
               const gchar * const envp[]);

gint
sydbox_execv (const SydboxContext * const ctx,
              const gchar * const path,
              const gchar * const argv[]);

gint
sydbox_execvp (const SydboxContext * const ctx,
               const gchar * const file,
               const gchar * const argv[]);

gint
sydbox_execve (const SydboxContext * const ctx,
               const gchar * const filename,
               const gchar * const argv[],
               const gchar * const envp[]);

G_END_DECLS

#endif

