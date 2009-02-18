/* Check program for t10-rename.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdio.h>
#include <stdlib.h>

enum test {
    T_FIRST,
    T_SECOND,
};

int main(int argc, char **argv) {
    int t = atoi(argv[1]);

    if (T_FIRST == t) {
        if (0 > rename("arnold.layne", "its.not.the.same"))
            return EXIT_FAILURE;
        else
            return EXIT_SUCCESS;
    }
    else if (T_SECOND == t) {
        if (0 > rename("arnold.layne", "/tmp/sydbox.txt"))
            return EXIT_FAILURE;
        else
            return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
