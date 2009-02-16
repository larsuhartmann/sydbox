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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "defs.h"

/* Fatal error. Print message and exit. */
void die(int err, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: ", PACKAGE);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);

    exit(err);
}

void lg(int level, const char *id, const char *fmt, ...) {
    va_list args;

    if (level <= log_level) {
        fprintf(stderr, "%s@%ld: [", PACKAGE, time(NULL));

        switch (level) {
            case LOG_ERROR:
                fprintf(stderr, "ERROR ");
                break;
            case LOG_WARNING:
                fprintf(stderr, "WARNING ");
                break;
            case LOG_NORMAL:
                fprintf(stderr, "NORMAL ");
                break;
            case LOG_VERBOSE:
                fprintf(stderr, "VERBOSE ");
                break;
            case LOG_DEBUG:
                fprintf(stderr, "DEBUG ");
                break;
        }
        fprintf(stderr, "%s] ", id);

        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);

        fputc('\n', stderr);
    }
}

void access_error(pid_t pid, const char *fmt, ...) {
    va_list args;
    time_t now;

    now = time(NULL);
    if (colour) {
        fprintf(stderr, "%s@%ld: " MAGENTA "Access violation!" NORMAL "\n", PACKAGE, now);
        fprintf(stderr, "%s@%ld: " MAGENTA "Child pid: " PINK "%i" NORMAL "\n", PACKAGE, now, pid);
        fprintf(stderr, "%s@%ld: " MAGENTA "Reason: " PINK, PACKAGE, now);
    }
    else {
        fprintf(stderr, "%s@%ld: Access violation!\n", PACKAGE, now);
        fprintf(stderr, "%s@%ld: Child pid: %i\n", PACKAGE, now, pid);
        fprintf(stderr, "%s@%ld: Reason: ", PACKAGE, now);
    }

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (colour)
        fprintf(stderr, NORMAL "\n");
    else
        fputc('\n', stderr);
}

void *xmalloc (size_t size) {
    void *t;

    if (0 == size)
        return NULL;

    t = malloc(size);
    if (NULL == t)
        die(EX_OSERR, "not enough memory");

    return t;
}

char *xstrndup (const char *s, size_t n) {
    char *t;

    if (NULL == s)
        die(EX_SOFTWARE, "bug in xstrndup call");

    t = xmalloc(n + 1);
    strncpy(t, s, n);
    t[n] = '\0';

    return t;
}

void bash_expand(const char *pathname, char *dest) {
    char command[32 + PATH_MAX] = "/bin/bash -c 'echo -n \"";
    strncat(command, pathname, PATH_MAX);
    strncat(command, "\"'", 2);
    FILE *bash = popen(command, "r");
    if (NULL == bash)
        die(EX_SOFTWARE, "bug in popen call: %s", strerror(errno));

    int i = 0;
    while (!feof(bash))
        dest[i++] = fgetc(bash);
    dest[i-1] = '\0';
    fclose(bash);
}
