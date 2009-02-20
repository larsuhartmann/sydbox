/* Check program for t14-truncate.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {
    if (0 > truncate("arnold.layne", 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
