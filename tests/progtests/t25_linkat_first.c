/* Check program for t25-linkat-first.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#define _ATFILE_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

int main(void) {
    DIR *dot = opendir(".");
    if (NULL == dot)
        return EXIT_FAILURE;
    int dfd = dirfd(dot);
    if (-1 == dfd)
        return EXIT_FAILURE;
    if (0 > linkat(dfd, "arnold.layne", AT_FDCWD, "arnold.layne.hard", 0))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
