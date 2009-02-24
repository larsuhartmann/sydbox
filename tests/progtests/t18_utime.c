/* Check program for t18-utime.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */


#include <sys/types.h>
#include <utime.h>
#include <stdlib.h>

int main(void) {
    struct utimbuf times;

    times.actime = 0;
    times.modtime = 0;

    if (0 > utime("arnold.layne", &times))
        return EXIT_FAILURE;
    else
        return EXIT_SUCCESS;
}
