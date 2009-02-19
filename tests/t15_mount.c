/* Check program for t15-mount.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <sys/mount.h>
#include <stdlib.h>

int main(void) {
    if (0 > mount("/dev", "see.emily.play",
                "pinkfs", MS_BIND, "")) {
        if (0 != errno) {
            /* Access denied by system not sydbox */
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}
