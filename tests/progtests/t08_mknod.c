/* Check program for t08-mknod.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    if (0 > mknod("arnold.layne.fifo", S_IFIFO, 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
