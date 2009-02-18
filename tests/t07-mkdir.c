/* Check program for t07-mkdir.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(void) {
    if (0 > mkdir("arnold.layne", 0644))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
