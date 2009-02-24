/* Check program for t19-unlink.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <unistd.h>
#include <stdlib.h>

int main(void) {
    if (0 > unlink("arnold.layne"))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
