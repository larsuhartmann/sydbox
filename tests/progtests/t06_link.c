/* Check program for t06-link.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    if (0 > link("see.emily.play/gnome", "arnold.layne.hard"))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
