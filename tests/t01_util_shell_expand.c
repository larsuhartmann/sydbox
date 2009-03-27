/* Check program for t01-util-bash-expand.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "../src/util.h"

int main(int argc, char **argv) {
    if (1 == argc)
        return EXIT_FAILURE;

    char *dest = shell_expand(argv[1]);
    printf("%s", dest);
    free(dest);
    return EXIT_SUCCESS;
}
