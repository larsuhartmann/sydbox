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

#define CMD_PATH                "/dev/sydbox/"
#define CMD_PATH_LEN            12
#define CMD_ON                  CMD_PATH"on"
#define CMD_ON_LEN              (CMD_PATH_LEN + 3)
#define CMD_OFF                 CMD_PATH"off"
#define CMD_OFF_LEN             (CMD_PATH_LEN + 4)
#define CMD_TOGGLE              CMD_PATH"toggle"
#define CMD_TOGGLE_LEN          (CMD_PATH_LEN + 7)
#define CMD_ENABLED             CMD_PATH"enabled"
#define CMD_ENABLED_LEN         (CMD_PATH_LEN + 8)
#define CMD_LOCK                CMD_PATH"lock"
#define CMD_LOCK_LEN            (CMD_PATH_LEN + 5)
#define CMD_EXEC_LOCK           CMD_PATH"exec_lock"
#define CMD_EXEC_LOCK_LEN       (CMD_PATH_LEN + 10)
#define CMD_WRITE               CMD_PATH"write/"
#define CMD_WRITE_LEN           (CMD_PATH_LEN + 6)
#define CMD_PREDICT             CMD_PATH"predict/"
#define CMD_PREDICT_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMWRITE             CMD_PATH"unwrite/"
#define CMD_RMWRITE_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMPREDICT           CMD_PATH"unpredict/"
#define CMD_RMPREDICT_LEN       (CMD_PATH_LEN + 10)
#define CMD_SANDBOX_EXEC        CMD_PATH"sandbox_exec"
#define CMD_SANDBOX_EXEC_LEN    (CMD_PATH_LEN + 13)
#define CMD_UNSANDBOX_EXEC      CMD_PATH"unsandbox_exec"
#define CMD_UNSANDBOX_EXEC_LEN  (CMD_PATH_LEN + 15)
#define CMD_ADDHOOK             CMD_PATH"addhook/"
#define CMD_ADDHOOK_LEN         (CMD_PATH_LEN + 8)
#define CMD_RMHOOK              CMD_PATH"rmhook/"
#define CMD_RMHOOK_LEN          (CMD_PATH_LEN + 7)

bool
path_magic_dir (const char *path);

bool
path_magic_on (const char *path);

bool
path_magic_off (const char *path);

bool
path_magic_toggle (const char *path);

bool
path_magic_enabled (const char *path);

bool
path_magic_lock (const char *path);

bool
path_magic_exec_lock (const char *path);

bool
path_magic_write (const char *path);

bool
path_magic_predict (const char *path);

bool
path_magic_rmwrite (const char *path);

bool
path_magic_rmpredict (const char *path);

bool
path_magic_sandbox_exec (const char *path);

bool
path_magic_unsandbox_exec (const char *path);

bool
path_magic_addhook(const char *path);

bool
path_magic_rmhook(const char *path);

int
pathnode_new (GSList **pathlist, const char *path, int sanitize);

int
pathnode_new_early (GSList **pathlist, const char *path, int sanitize);

void
pathnode_free (GSList **pathlist);

void
pathnode_delete (GSList **pathlist, const char *path_sanitized);

int
pathlist_init (GSList **pathlist, const char *pathlist_env);

int
pathlist_check (GSList *pathlist, const char *path_sanitized);

#endif // SYDBOX_GUARD_PATH_H

