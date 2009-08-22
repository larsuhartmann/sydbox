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

#ifndef SYDBOX_GUARD_PATH_H
#define SYDBOX_GUARD_PATH_H 1

#include <stdbool.h>

#include <glib.h>

#include "commands.h"

bool path_magic_dir(const char *path);

bool path_magic_on(const char *path);

bool path_magic_off(const char *path);

bool path_magic_toggle(const char *path);

bool path_magic_enabled(const char *path);

bool path_magic_lock(const char *path);

bool path_magic_exec_lock(const char *path);

bool path_magic_write(const char *path);

bool path_magic_rmwrite(const char *path);

bool path_magic_sandbox_exec(const char *path);

bool path_magic_unsandbox_exec(const char *path);

bool path_magic_addfilter(const char *path);

bool path_magic_rmfilter(const char *path);

int pathnode_new(GSList **pathlist, const char *path, int sanitize);

int pathnode_new_early(GSList **pathlist, const char *path, int sanitize);

void pathnode_free(GSList **pathlist);

void pathnode_delete(GSList **pathlist, const char *path_sanitized);

int pathlist_init(GSList **pathlist, const char *pathlist_env);

int pathlist_check(GSList *pathlist, const char *path_sanitized);

#endif // SYDBOX_GUARD_PATH_H

