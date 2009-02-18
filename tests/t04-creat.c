/* Check program for t04-creat.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

int main(void) {
    if (0 > creat("arnold.layne", 0644))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
