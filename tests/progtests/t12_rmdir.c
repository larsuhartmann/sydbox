/* Check program for t12-rmdir.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <unistd.h>

int main(void) {
    if (0 > rmdir("see.emily.play"))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
