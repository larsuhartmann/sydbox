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

#include <glib.h>

#include "hooker.h"

gint hooker_spawn(const gchar *hook, pid_t pid, const gchar *sname, const gchar *path)
{
    gint ret;
    GError *spawn_error;
    gchar *argv[5];

    argv[0] = g_strdup(hook);
    argv[1] = g_strdup_printf("%i", pid);
    argv[2] = g_strdup(sname);
    argv[3] = path ? g_strdup(path) : NULL;
    argv[4] = NULL;

    spawn_error = NULL;
    if (G_UNLIKELY(!g_spawn_sync(NULL, argv, NULL, 0, NULL, NULL, NULL, NULL, &ret, &spawn_error))) {
        g_warning("failed to execute hook `%s': %s", argv[0], spawn_error->message);
        g_error_free(spawn_error);
        for (int i = 0; i < 5; i++)
            g_free(argv[i]);
        return 0;
    }

    for (int i = 0; i < 5; i++)
        g_free(argv[i]);
    g_debug("hook `%s' returned %d", argv[0], ret);
    return ret;
}

