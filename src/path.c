/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "path.h"
#include "sydbox-log.h"
#include "sydbox-utils.h"


static char *
shell_expand (const char * const str)
{
    gchar *argv[4] = { "/bin/sh", "-c", NULL, NULL };
    gchar *quoted, *output = NULL;
    GError *error = NULL;
    gint retval, nlindex;

    quoted = g_shell_quote (str);
    argv[2] = g_strdup_printf ("echo '%s'", quoted);
    g_free (quoted);

    if (G_UNLIKELY(! g_spawn_sync (NULL, argv, NULL, G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
                    &output, NULL, &retval, &error))) {
        g_printerr ("failed to expand `%s': %s", str, error->message);
        g_error_free (error);
    }

    g_free (argv[2]);

    // Lose the newline at the end.
    nlindex = strlen(output) - 1;
    assert('\n' == output[nlindex]);
    output[nlindex] = '\0';
    return output;
}


inline bool path_magic_dir(const char *path) {
    return (0 == strncmp(path, CMD_PATH, CMD_PATH_LEN - 1));
}

inline bool path_magic_on(const char *path) {
    return (0 == strncmp(path, CMD_ON, CMD_ON_LEN));
}

inline bool path_magic_off(const char *path) {
    return  (0 == strncmp(path, CMD_OFF, CMD_OFF_LEN));
}

inline bool path_magic_toggle(const char *path) {
    return (0 == strncmp(path, CMD_TOGGLE, CMD_TOGGLE_LEN));
}

inline bool path_magic_enabled(const char *path) {
    return (0 == strncmp(path, CMD_ENABLED, CMD_ENABLED_LEN));
}

inline bool path_magic_lock(const char *path) {
    return (0 == strncmp(path, CMD_LOCK, CMD_LOCK_LEN));
}

inline bool path_magic_exec_lock(const char *path) {
    return (0 == strncmp(path, CMD_EXEC_LOCK, CMD_EXEC_LOCK_LEN));
}

inline bool path_magic_write(const char *path) {
    return (0 == strncmp(path, CMD_WRITE, CMD_WRITE_LEN));
}

inline bool path_magic_predict(const char *path) {
    return (0 == strncmp(path, CMD_PREDICT, CMD_PREDICT_LEN));
}

inline bool path_magic_rmwrite(const char *path) {
    return (0 == strncmp(path, CMD_RMWRITE, CMD_RMWRITE_LEN));
}

inline bool path_magic_rmpredict(const char *path) {
    return (0 == strncmp(path, CMD_RMPREDICT, CMD_RMPREDICT_LEN));
}

inline bool path_magic_ban_exec(const char *path)
{
    return (0 == strncmp(path, CMD_BAN_EXEC, CMD_BAN_EXEC_LEN));
}

inline bool path_magic_unban_exec(const char *path)
{
    return (0 == strncmp(path, CMD_UNBAN_EXEC, CMD_UNBAN_EXEC_LEN));
}

int pathnode_new(GSList **pathlist, const char *path, int sanitize) {
    char *data;

    if (G_UNLIKELY(NULL == path)) {
        g_info ("path is NULL not adding to list");
        return -1;
    }
    else if (G_UNLIKELY(0 == strlen(path))) {
        g_info ("path is empty not adding to list");
        return -1;
    }

    if (G_LIKELY(!sanitize))
        data = g_strdup (path);
    else {
        char *spath = sydbox_compress_path (path);
        data = shell_expand (spath);
        g_free (spath);
        g_info ("new path item `%s'", data);
    }
    *pathlist = g_slist_prepend(*pathlist, data);
    return 0;
}

int pathnode_new_early(GSList **pathlist, const char *path, int sanitize)
{
    char *data, *spath;

    if (G_UNLIKELY(NULL == path || 0 == strlen(path)))
        return -1;

    if (!sanitize)
        data = g_strdup(path);
    else {
        spath = sydbox_compress_path (path);
        data = shell_expand(spath);
        g_free(spath);
    }
    *pathlist = g_slist_prepend(*pathlist, data);
    return 0;
}

void pathnode_free(GSList **pathlist) {
    g_slist_foreach(*pathlist, (GFunc) g_free, NULL);
    g_slist_free(*pathlist);
    *pathlist = NULL;
}

void pathnode_delete(GSList **pathlist, const char *path_sanitized) {
    GSList *walk;

    walk = *pathlist;
    while (NULL != walk) {
        if (0 == strncmp(walk->data, path_sanitized, strlen(path_sanitized) + 1)) {
            g_debug ("freeing pathnode %p", (void *) walk);
            *pathlist = g_slist_remove_link(*pathlist, walk);
            g_free(walk->data);
            g_slist_free(walk);
            break;
        }
        walk = g_slist_next(walk);
    }
}

int pathlist_init(GSList **pathlist, const char *pathlist_env) {
    char **split;
    int nempty, npaths;

    if (NULL == pathlist_env) {
        g_info ("the given environment variable isn't set");
        return 0;
    }

    nempty = 0;
    split = g_strsplit(pathlist_env, ":", -1);
    for (unsigned int i = 0; i < g_strv_length(split); i++) {
        if (0 != strncmp(split[i], "", 2))
            *pathlist = g_slist_prepend(*pathlist, g_strdup(split[i]));
        else {
            g_debug ("ignoring empty path element in position %d", i);
            ++nempty;
        }
    }
    npaths = g_strv_length(split) - nempty;
    g_strfreev(split);
    g_info ("initialized path list with %d paths", npaths);
    return npaths;
}

int pathlist_check(GSList *pathlist, const char *path_sanitized) {
    int ret;
    GSList *walk;

    g_debug ("checking `%s'", path_sanitized);

    ret = 0;
    walk = pathlist;
    while (NULL != walk) {
        if (0 == strncmp(walk->data, "/", 2)) {
            g_debug ("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
            ret = 1;
            break;
        }
        else if (0 == strncmp(path_sanitized, walk->data, strlen(walk->data))) {
            if (strlen(path_sanitized) > strlen(walk->data)) {
                /* Path begins with one of the allowed paths. Check for a
                 * zero byte or a / on the next character so that for example
                 * /devzero/foo doesn't pass the test when /dev is the only
                 * allowed path.
                 */
                const char last = path_sanitized[strlen(walk->data)];
                if ('\0' == last || '/' == last) {
                    g_debug ("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
                    ret = 1;
                    break;
                }
                else
                    g_debug ("`%s' doesn't begin with `%s'", path_sanitized, (char *) walk->data);
            }
            else {
                g_debug ("`%s' begins with `%s'", path_sanitized, (char *) walk->data);
                ret = 1;
                break;
            }
        }
        else
            g_debug ("`%s' doesn't begin with `%s'", path_sanitized, (char *) walk->data);
        walk = g_slist_next(walk);
    }
    if (ret)
        g_debug ("path list check succeeded for `%s'", path_sanitized);
    else
        g_debug ("path list check failed for `%s'", path_sanitized);
    return ret;
}

