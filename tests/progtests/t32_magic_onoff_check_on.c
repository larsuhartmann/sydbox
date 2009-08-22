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
    struct stat buf;

    /* Turn off the sandbox. */
    if (0 > stat("/dev/sydbox/off", &buf)) {
        fprintf(stderr, "%s: failed to set sydbox off\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Turn it on again */
    if (0 > stat("/dev/sydbox/on", &buf)) {
        fprintf(stderr, "%s: failed to set sydbox on\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* This should throw an access violation */
    fd = open("arnold.layne", O_WRONLY);
    if (0 > fd)
        return (EPERM == errno) ? EXIT_SUCCESS : EXIT_FAILURE;
    else {
        /* No access violation? try to write to file */
        if (0 > write(fd, "hello arnold layne!", 20))
            return (EPERM == errno) ? EXIT_SUCCESS : EXIT_FAILURE;
        close(fd);
    }
    return EXIT_SUCCESS;
}
