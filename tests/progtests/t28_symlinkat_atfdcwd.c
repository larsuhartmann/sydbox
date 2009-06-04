/* Check program for t28-symlinkat-atfdcwd.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    if (0 > symlinkat("see.emily.play/gnome", AT_FDCWD, "jugband.blues"))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
