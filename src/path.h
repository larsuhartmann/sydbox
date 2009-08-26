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

#define CMD_PATH                        "/dev/sydbox/"
#define CMD_PATH_LEN                    12
#define CMD_ON                          CMD_PATH"on"
#define CMD_ON_LEN                      (CMD_PATH_LEN + 3)
#define CMD_OFF                         CMD_PATH"off"
#define CMD_OFF_LEN                     (CMD_PATH_LEN + 4)
#define CMD_TOGGLE                      CMD_PATH"toggle"
#define CMD_TOGGLE_LEN                  (CMD_PATH_LEN + 7)
#define CMD_ENABLED                     CMD_PATH"enabled"
#define CMD_ENABLED_LEN                 (CMD_PATH_LEN + 8)
#define CMD_LOCK                        CMD_PATH"lock"
#define CMD_LOCK_LEN                    (CMD_PATH_LEN + 5)
#define CMD_EXEC_LOCK                   CMD_PATH"exec_lock"
#define CMD_EXEC_LOCK_LEN               (CMD_PATH_LEN + 10)
#define CMD_WRITE                       CMD_PATH"write/"
#define CMD_WRITE_LEN                   (CMD_PATH_LEN + 6)
#define CMD_RMWRITE                     CMD_PATH"unwrite/"
#define CMD_RMWRITE_LEN                 (CMD_PATH_LEN + 8)
#define CMD_SANDBOX_EXEC                CMD_PATH"sandbox/exec"
#define CMD_SANDBOX_EXEC_LEN            (CMD_PATH_LEN + 13)
#define CMD_SANDUNBOX_EXEC              CMD_PATH"sandunbox/exec"
#define CMD_SANDUNBOX_EXEC_LEN          (CMD_PATH_LEN + 15)
#define CMD_SANDBOX_NET                 (CMD_PATH"sandbox/net")
#define CMD_SANDBOX_NET_LEN             (CMD_PATH_LEN + 12)
#define CMD_SANDUNBOX_NET               (CMD_PATH"sandunbox/net")
#define CMD_SANDUNBOX_NET_LEN           (CMD_PATH_LEN + 14)
#define CMD_ADDFILTER                   CMD_PATH"addfilter/"
#define CMD_ADDFILTER_LEN               (CMD_PATH_LEN + 10)
#define CMD_RMFILTER                    CMD_PATH"rmfilter/"
#define CMD_RMFILTER_LEN                (CMD_PATH_LEN + 9)
#define CMD_NET_ALLOW                   CMD_PATH"net/allow"
#define CMD_NET_ALLOW_LEN               (CMD_PATH_LEN + 10)
#define CMD_NET_DENY                    CMD_PATH"net/deny"
#define CMD_NET_DENY_LEN                (CMD_PATH_LEN + 9)
#define CMD_NET_LOCAL                   CMD_PATH"net/local"
#define CMD_NET_LOCAL_LEN               (CMD_PATH_LEN + 10)
#define CMD_NET_RESTRICT_CONNECT        CMD_PATH"net/restrict/connect"
#define CMD_NET_RESTRICT_CONNECT_LEN    (CMD_PATH_LEN + 21)
#define CMD_NET_UNRESTRICT_CONNECT      CMD_PATH"net/unrestrict/connect"
#define CMD_NET_UNRESTRICT_CONNECT_LEN  (CMD_PATH_LEN + 23)
#define CMD_NET_WHITELIST               CMD_PATH"net/whitelist/"
#define CMD_NET_WHITELIST_LEN           (CMD_PATH_LEN + 14)

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

bool path_magic_sandunbox_exec(const char *path);

bool path_magic_sandbox_net(const char *path);

bool path_magic_sandunbox_net(const char *path);

bool path_magic_addfilter(const char *path);

bool path_magic_rmfilter(const char *path);

bool path_magic_net_allow(const char *path);

bool path_magic_net_deny(const char *path);

bool path_magic_net_local(const char *path);

bool path_magic_net_restrict_connect(const char *path);

bool path_magic_net_unrestrict_connect(const char *path);

bool path_magic_net_whitelist(const char *path);

int pathnode_new(GSList **pathlist, const char *path, int sanitize);

int pathnode_new_early(GSList **pathlist, const char *path, int sanitize);

void pathnode_free(GSList **pathlist);

void pathnode_delete(GSList **pathlist, const char *path_sanitized);

int pathlist_init(GSList **pathlist, const char *pathlist_env);

int pathlist_check(GSList *pathlist, const char *path_sanitized);

#endif // SYDBOX_GUARD_PATH_H

