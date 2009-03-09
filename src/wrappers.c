/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgen.h>
#if defined(basename)
#undef basename // get the GNU version from string.h
#endif
#include <string.h>

#include "defs.h"

// dirname wrapper which doesn't modify its argument
char *edirname(const char *path) {
    char *pathc = xstrndup(path, strlen(path) + 1);
    char *dname = dirname(pathc);
    char *dnamec = xstrndup(dname, strlen(dname) + 1);
    free(pathc);
    return dnamec;
}

char *ebasename(const char *path) {
    return basename(path);
}

char *egetcwd(void) {
    return get_current_dir_name();
}

// readlink that allocates the string itself and appends a zero at the end
char *ereadlink(const char *path) {
    char *buf;
    int nrequested, nwritten;

    buf = NULL;
    nrequested = 32;
    for (;;) {
        buf = xrealloc(buf, nrequested);
        nwritten = readlink(path, buf, nrequested);
        if (0 > nwritten)
            return NULL;
        else if (nrequested > nwritten)
            break;
        else
            nrequested *= 2;
    }
    buf[nwritten] = '\0';
    return buf;
}

char *pgetcwd(pid_t pid) {
    char procfd[64];
    snprintf(procfd, 64, "/proc/%i/cwd", pid);
    return ereadlink(procfd);
}
