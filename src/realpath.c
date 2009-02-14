/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * realpath.c -- canonicalize pathname by removing symlinks
 * Copyright (C) 1993 Rick Sladkey <jrs@world.std.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library Public License for more details.
 */

/* Grabbed from util-linux-ng-2.14.1
 * Changed myrealpath to safe_realpath
 * Removed resolved_path argument. Uses xmalloc to allocate it.
 * Removed maxreslth argument, we use PATH_MAX + 1 instead.
 * Added resolve argument which can be set to resolve symlinks when needed.
 * Added issymlink argument which can be used to retrieve whether the path was
 * a symlink.
 * Added function getcwd_pid() to get the current working directory of the
 * child. Added pid argument to safe_realpath to specify pid.
 */

/*
 * IMPORTANT: It's of crucial importance that these functions do the right
 * thing.
 */

/*
 * This routine is part of libc.  We include it nevertheless,
 * since the libc version has some security flaws.
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "defs.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef MAXSYMLINKS
#define MAXSYMLINKS 256
#endif

/* TODO is there a better way to do this? */
char *getcwd_pid(char *buf, size_t size, pid_t pid) {
    int n;
    char cwd[PATH_MAX];

    snprintf(cwd, PATH_MAX, "/proc/%i/cwd", pid);
    n = readlink(cwd, buf, size - 1);
    if (0 > n)
        return NULL;
    /* Readlink does NOT append a NULL byte to buf. */
    buf[n] = '\0';
    return buf;
}

char *safe_realpath(const char *path, pid_t pid, int resolve, int *issymlink) {
    int readlinks = 0;
    char *npath, *resolved_path;
    char link_path[PATH_MAX + 1];
    int n;
    char *buf = NULL;

    resolved_path = xmalloc(PATH_MAX + 2);
    npath = resolved_path;

    /* If it's a relative pathname use getcwd for starters. */
    if (*path != '/') {
        if (!getcwd_pid(npath, PATH_MAX - 1, pid))
            return NULL;
        npath += strlen(npath);
        if (npath[-1] != '/')
            *npath++ = '/';
    } else {
        *npath++ = '/';
        path++;
    }

    /* Expand each slash-separated pathname component. */
    while (*path != '\0') {
        /* Ignore stray "/" */
        if (*path == '/') {
            path++;
            continue;
        }
        if (*path == '.' && (path[1] == '\0' || path[1] == '/')) {
            /* Ignore "." */
            path++;
            continue;
        }
        if (*path == '.' && path[1] == '.' &&
            (path[2] == '\0' || path[2] == '/')) {
            /* Backup for ".." */
            path += 2;
            while (npath > resolved_path+1 &&
                   (--npath)[-1] != '/')
                    ;
            continue;
        }
        /* Safely copy the next pathname component. */
        while (*path != '\0' && *path != '/') {
            if (npath-resolved_path > PATH_MAX - 1) {
                errno = ENAMETOOLONG;
                goto err;
            }
            *npath++ = *path++;
        }

        /* Protect against infinite loops. */
        if (readlinks++ > MAXSYMLINKS) {
            errno = ELOOP;
            goto err;
        }

        if (!resolve) {
            *npath++ = '/';
            continue;
        }

        /* See if last pathname component is a symlink. */
        *npath = '\0';

        n = readlink(resolved_path, link_path, PATH_MAX);
        if (n < 0) {
            /* EINVAL means the file exists but isn't a symlink. */
            if (errno != EINVAL)
                goto err;
            else if (NULL != issymlink)
                *issymlink = 0;
        } else {
            if (NULL != issymlink)
                *issymlink = 1;
            int m;
            char *newbuf;

            /* Note: readlink doesn't add the null byte. */
            link_path[n] = '\0';
            if (*link_path == '/')
                /* Start over for an absolute symlink. */
                npath = resolved_path;
            else
                /* Otherwise back up over this component. */
                while (*(--npath) != '/')
                        ;

            /* Insert symlink contents into path. */
            m = strlen(path);
            newbuf = xmalloc(m + n + 1);
            memcpy(newbuf, link_path, n);
            memcpy(newbuf + n, path, m + 1);
            free(buf);
            path = buf = newbuf;
        }
        *npath++ = '/';
    }
    /* Delete trailing slash but don't whomp a lone slash. */
    if (npath != resolved_path+1 && npath[-1] == '/')
        npath--;
    /* Make sure it's null terminated. */
    *npath = '\0';

    free(buf);
    return resolved_path;
err:
    free(buf);
    free(resolved_path);
    return NULL;
}
