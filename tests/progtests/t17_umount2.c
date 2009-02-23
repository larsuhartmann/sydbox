/* Check program for t17-umount2.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <sys/mount.h>
#include <stdlib.h>

int main(void) {
    if (0 > umount2("see.emily.play", 0)) {
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}
