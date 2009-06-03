/* Check program for t14-truncate.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    char *long_dir, *fname;

    if (3 > argc)
        return EXIT_FAILURE;
    else {
        long_dir = argv[1];
        fname = argv[2];
    }

    for (int i = 0; i < 64; i++) {
        if (0 > chdir(long_dir))
            return EXIT_FAILURE;
    }

    if (0 > truncate(fname, 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
