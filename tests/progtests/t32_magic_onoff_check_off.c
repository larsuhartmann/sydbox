/* Check program for t32-magic-onoff.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int fd;

    /* Turn off the sandbox. */
    if (0 > open("/dev/sydbox/off", O_RDWR)) {
        fprintf(stderr, "%s: failed to set sydbox off\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open("arnold.layne", O_WRONLY);
    if (0 > fd) {
        fprintf(stderr, "%s: failed to open arnold.layne: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    if (0 > write(fd, "hello arnold layne!", 20)) {
        fprintf(stderr, "%s: failed to write to arnold.layne: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
