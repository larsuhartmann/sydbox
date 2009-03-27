/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __PATH_H__
#define __PATH_H__

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

bool
path_magic_dir (const char *path);

bool
path_magic_on (const char *path);

bool
path_magic_off (const char *path);

bool
path_magic_toggle (const char *path);

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

int
pathnode_new (GSList **pathlist, const char *path, int sanitize);

void
pathnode_free (GSList **pathlist);

void
pathnode_delete (GSList **pathlist, const char *path_sanitized);

int
pathlist_init (GSList **pathlist, const char *pathlist_env);

int
pathlist_check (GSList *pathlist, const char *path_sanitized);

#endif

