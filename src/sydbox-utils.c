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

#include <stdbool.h>
#include <string.h>

#include <glib/gstdio.h>

#include "sydbox-log.h"
#include "sydbox-utils.h"
#include "sydbox-config.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void
sydbox_access_violation (const pid_t pid, const gchar *fmt, ...)
{
    va_list args;
    time_t now = time (NULL);

    g_fprintf (stderr, PACKAGE "@%lu: %sAccess Violation!%s\n", now,
               sydbox_config_get_colourise_output () ? ANSI_MAGENTA : "",
               sydbox_config_get_colourise_output () ? ANSI_NORMAL : "");
    g_fprintf (stderr, PACKAGE "@%lu: %sChild Process ID: %s%i%s\n", now,
               sydbox_config_get_colourise_output () ? ANSI_MAGENTA : "",
               sydbox_config_get_colourise_output () ? ANSI_DARK_MAGENTA : "",
               pid,
               sydbox_config_get_colourise_output () ? ANSI_NORMAL : "");
    g_fprintf (stderr, PACKAGE "@%lu: %sReason: %s", now,
               sydbox_config_get_colourise_output () ? ANSI_MAGENTA : "",
               sydbox_config_get_colourise_output () ? ANSI_DARK_MAGENTA : "");

    va_start (args, fmt);
    g_vfprintf (stderr, fmt, args);
    va_end (args);

    g_fprintf (stderr, "%s\n", sydbox_config_get_colourise_output () ? ANSI_NORMAL : "");
}

gchar *
sydbox_compress_path (const gchar * const path)
{
    bool skip_slashes = false;
    gchar *retval;
    GString *compressed;
    guint i;

    compressed = g_string_sized_new (strlen (path));

    for (i = 0; i < strlen (path); i++) {
        if (path[i] == '/' && skip_slashes)
            continue;
        skip_slashes = (path[i] == '/');

        g_string_append_c (compressed, path[i]);
    }

    /* truncate trailing slashes on paths other than '/' */
    if (compressed->len > 1 && compressed->str[compressed->len - 1] == '/')
        g_string_truncate (compressed, compressed->len - 1);

    retval = compressed->str;
    g_string_free (compressed, FALSE);

    return retval;
}

