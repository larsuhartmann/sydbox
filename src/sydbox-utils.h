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

#ifndef SYDBOX_GUARD_UTILS_H
#define SYDBOX_GUARD_UTILS_H 1

#include <glib.h>

/**
 * ANSI_:
 * @ANSI_NORMAL: reset style
 * @ANSI_MAGENTA: set foreground to magenta
 * @ANSI_DARK_MAGENTA: set foreground to dark magenta
 *
 * ANSI escape sequences
 *
 * Since: 0.1_alpha
 **/
#define ANSI_NORMAL         "[00;00m"
#define ANSI_MAGENTA        "[00;35m"
#define ANSI_DARK_MAGENTA   "[01;35m"

/**
 * sydbox_access_violation:
 * @pid: process id of the process being traced
 * @path: path that caused the access violation if any.
 * @fmt: format string (as with printf())
 * @varargs: parameters to be used with @fmt
 *
 * Raise an access violation.
 *
 * Since: 0.1_alpha
 **/
void sydbox_access_violation(const pid_t pid, const gchar *path, const gchar *fmt, ...) G_GNUC_PRINTF (3, 4);

/**
 * sydbox_compress_path:
 * @path: the path to compress
 *
 * Replaces runs of forward slashses with a single slash.  This does not
 * canonicalise the path!
 *
 * Returns: string containing the compressed path.  The return value should be
 * freed with g_free when no longer in use.
 *
 * Since: 0.1_alpha
 **/
gchar *sydbox_compress_path(const gchar * const path);

#endif // SYDBOX_GUARD_UTILS_H

