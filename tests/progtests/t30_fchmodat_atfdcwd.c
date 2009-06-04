/* Check program for t30-fchmodat-atfdcwd.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

int main(void) {
    if (0 > fchmodat(AT_FDCWD, "arnold.layne", 0000, 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
