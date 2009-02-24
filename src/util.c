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
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

void _die(int err, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "%s: ", PACKAGE);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);

    _exit(err);
}

void lg(int level, const char *funcname, const char *fmt, ...) {
    static int log_file_opened = 0;
    static int isstderr;
    va_list args;

    isstderr = '\0' == log_file[0] ? 1 : 0;

    if (!log_file_opened && !isstderr) {
        flog = fopen(log_file, "a");
        if (NULL == flog)
            die(EX_SOFTWARE, "Failed to open log file %s: %s",
                    log_file, strerror(errno));
        log_file_opened = 1;
    }

    if (level <= log_level) {
        if (isstderr)
            fprintf(stderr, PACKAGE"@%ld: [", time(NULL));
        else
            fprintf(flog, "%ld: [", time(NULL));

        switch (level) {
            case LOG_ERROR:
                if (isstderr)
                    fprintf(stderr, "ERROR ");
                else
                    fprintf(flog, "ERROR ");
                break;
            case LOG_WARNING:
                if (isstderr)
                    fprintf(stderr, "WARNING ");
                else
                    fprintf(flog, "WARNING ");
                break;
            case LOG_NORMAL:
                if (isstderr)
                    fprintf(stderr, "NORMAL ");
                else
                    fprintf(flog, "NORMAL ");
                break;
            case LOG_VERBOSE:
                if (isstderr)
                    fprintf(stderr, "VERBOSE ");
                else
                    fprintf(flog, "VERBOSE ");
                break;
            case LOG_DEBUG:
                if (isstderr)
                    fprintf(stderr, "DEBUG ");
                else
                    fprintf(flog, "DEBUG ");
                break;
            case LOG_DEBUG_CRAZY:
                if (isstderr)
                    fprintf(stderr, "CRAZY ");
                else
                    fprintf(flog, "CRAZY ");
        }
        if (isstderr)
            fprintf(stderr, "%s] ", funcname);
        else
            fprintf(flog, "%s] ", funcname);

        va_start(args, fmt);
        if (isstderr)
            vfprintf(stderr, fmt, args);
        else
            vfprintf(flog, fmt, args);
        va_end(args);

        if (isstderr)
            fputc('\n', stderr);
        else
            fputc('\n', flog);
    }
}

void access_error(pid_t pid, const char *fmt, ...) {
    va_list args;
    time_t now;

    now = time(NULL);
    if (colour) {
        fprintf(stderr, PACKAGE"@%ld: "MAGENTA"Access violation!"NORMAL"\n", now);
        fprintf(stderr, PACKAGE"@%ld: "MAGENTA"Child pid: "PINK"%i"NORMAL"\n", now, pid);
        fprintf(stderr, PACKAGE"@%ld: "MAGENTA"Reason: "PINK, now);
    }
    else {
        fprintf(stderr, PACKAGE"@%ld: Access violation!\n", now);
        fprintf(stderr, PACKAGE"@%ld: Child pid: %i\n", now, pid);
        fprintf(stderr, PACKAGE"@%ld: Reason: ", now);
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

int remove_slash(const char *pathname, char *dest) {
    int gotslash = 0, nslashes = 0;

    for (int i = 0, j = 0; i < PATH_MAX; i++) {
        if ('/' == pathname[i]) {
            if (gotslash) {
                ++nslashes;
                continue;
            }
            else
                gotslash = 1;
        }
        else
            gotslash = 0;
        dest[j++] = pathname[i];
        /* Remove trailing slash */
        if ('\0' == pathname[i]) {
            if ('/' == dest[j - 2]) {
                ++nslashes;
                dest[j - 2] = '\0';
            }
            break;
        }
    }
    if (nslashes)
        LOGD("Simplified pathname \"%s\" to \"%s\"", pathname, dest);
    return nslashes;
}

void shell_expand(const char *pathname, char *dest) {
    char command[32 + PATH_MAX] = "/bin/sh -c 'echo -n \"";
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
    if (0 != strncmp(pathname, dest, PATH_MAX))
        LOGD("Expanded path \"%s\" to \"%s\" using /bin/sh", pathname, dest);
}

/* A wrapper around safe_realpath */
char *resolve_path(const char *path, pid_t pid, int resolve, int *issymlink) {
    char *rpath;

    if (resolve)
        rpath = safe_realpath(path, pid, 1, issymlink);
    else
        rpath = safe_realpath(path, pid, 0, NULL);

    if (NULL == rpath) {
        if (ENOENT == errno) {
            /* File doesn't exist, check the directory */
            char *dirc, *dname;
            dirc = xstrndup(path, PATH_MAX);
            dname = dirname(dirc);
            LOGD("File \"%s\" doesn't exist, using directory \"%s\"", path, dname);
            if (resolve)
                rpath = safe_realpath(dname, pid, 1, issymlink);
            else
                rpath = safe_realpath(dname, pid, 0, NULL);
            free(dirc);

            if (NULL == rpath) {
                /* Neither file nor the directory exists */
                LOGD("Directory doesn't exist as well, setting errno to ENOENT");
                errno = ENOENT;
                return NULL;
            }
            else {
                /* Add the directory back */
                LOGD("File \"%s\" doesn't exist but directory \"%s\" exists, adding basename",
                        path, rpath);
                char *basec, *bname;
                basec = xstrndup(path, PATH_MAX);
                bname = basename(basec);
                strncat(rpath, "/", 1);
                strncat(rpath, bname, strlen(bname));
                free(basec);
            }
        }
        else {
            // LOGW("safe_realpath() failed for \"%s\": %s", path, strerror(errno));
            return NULL;
        }
    }
    errno = 0;
    return rpath;
}
