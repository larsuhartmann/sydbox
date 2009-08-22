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

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>

#include "commands.h"

static bool has_sydbox(void)
{
    struct stat buf;
    return (0 == stat(SYDBOX_CMD_PATH, &buf));
}

static int cmd_on(void)
{
    return open(SYDBOX_CMD_ON, O_WRONLY);
}

static int cmd_off(void)
{
    return open(SYDBOX_CMD_OFF, O_WRONLY);
}

static int cmd_enabled(void)
{
    struct stat buf;
    return stat(SYDBOX_CMD_ENABLED, &buf);
}

int main(int argc, char **argv)
{
    const char *mode;

    if (!has_sydbox()) {
        g_printerr("fatal: not running under sydbox\n");
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        g_printerr("fatal: command not specified\n");
        return EXIT_FAILURE;
    }
    mode = argv[1];

    if (0 == strncmp(mode, "on", 3))
        return cmd_on();
    else if (0 == strncmp(mode, "off", 4))
        return cmd_off();
    if (0 == strncmp(mode, "enabled", 8))
        return cmd_enabled();
    else {
        g_printerr("fatal: unknown command `%s'\n", mode);
        return EXIT_FAILURE;
    }
}

