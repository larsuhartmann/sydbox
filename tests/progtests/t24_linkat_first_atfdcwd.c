/* Check program for t24-linkat-first-atfdcwd.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(void) {
    if (0 > linkat(AT_FDCWD, "arnold.layne", AT_FDCWD, "its.not.the.same", 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
