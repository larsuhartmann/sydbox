/* Check program for t11-rename-second.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    if (0 > rename("see.emily.play/gnome", "jugband.blues"))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
