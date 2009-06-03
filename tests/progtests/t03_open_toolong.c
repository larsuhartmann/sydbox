/* Check program for t03-open.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

enum test {
    T_READONLY,
    T_WRONLY,
    T_RDWR,
};

int main(int argc, char **argv) {
    int fd;
    int t = atoi(argv[1]);
    char *long_dir = argv[2];
    char *fname = argv[3];

    for (int i = 0; i < 64; i++) {
        if (0 > chdir(long_dir))
            return EXIT_FAILURE;
    }

    switch (t) {
        case T_READONLY:
            if (0 > open(fname, O_RDONLY))
                return EXIT_FAILURE;
            else
                return EXIT_SUCCESS;
        case T_WRONLY:
            fd = open(fname, O_WRONLY);
            if (0 > fd)
                return EXIT_FAILURE;
            else {
                write(fd, "why can't you see?", 18);
                close(fd);
                return EXIT_SUCCESS;
            }
        case T_RDWR:
            fd = open(fname, O_RDWR);
            if (0 > fd)
                return EXIT_FAILURE;
            else {
                write(fd, "why can't you see?", 18);
                close(fd);
                return EXIT_SUCCESS;
            }

    }
    return EXIT_FAILURE;
}
