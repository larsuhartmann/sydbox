/* Check program for t32-magic-onoff.bash
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    struct stat buf;
    return (0 > stat("/dev/sydbox/off", &buf)) ? EXIT_FAILURE : EXIT_SUCCESS;
}
