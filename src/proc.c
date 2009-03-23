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

#include <errno.h>
#include <stdio.h>

#include "defs.h"

char *pgetcwd(pid_t pid) {
    char *cwd;
    char linkcwd[64];
    snprintf(linkcwd, 64, "/proc/%i/cwd", pid);

    // First try ereadlink()
    cwd = ereadlink(linkcwd);
    if (NULL != cwd)
        return cwd;
    else if (ENAMETOOLONG != errno)
        return NULL;

    // Now try egetcwd()
    errno = 0;
    if (0 > echdir(linkcwd))
        return NULL;
    return egetcwd();
}

char *pgetdir(pid_t pid, int dirfd) {
    char procfd[128];
    snprintf(procfd, 128, "/proc/%i/fd/%d", pid, dirfd);
    return ereadlink(procfd);
}
