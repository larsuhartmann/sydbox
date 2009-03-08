/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 * Based in part upon sandbox-1.3.7 which is:
 *  Copyright 1999-2008 Gentoo Foundation
 *  Copyright 2004-2007 Martin Schlemmer <azarah@nosferatu.za.org>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "defs.h"

int colour = -1;
int log_level = -1;
char log_file[PATH_MAX] = { 0 };
FILE *log_fp = NULL;

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

void lg(int level, const char *func, size_t line, const char *fmt, ...) {
    static int log_file_opened = 0;
    va_list args;

    if (!log_file_opened) {
        int isstderr = '\0' == log_file[0] ? 1 : 0;

        if (isstderr)
            log_fp = stderr;
        else {
            log_fp = fopen(log_file, "a");
            if (NULL == log_fp)
                DIESOFT("Failed to open log file \"%s\": %s", log_file, strerror(errno));
        }
        log_file_opened = 1;
    }

    if (NULL == log_fp)
        return;
    else if (level > log_level)
        return;

    fprintf(log_fp, PACKAGE"@%ld: [", time(NULL));
    switch (level) {
        case LOG_ERROR:
            fprintf(log_fp, "ERROR ");
            break;
        case LOG_WARNING:
            fprintf(log_fp, "WARNING ");
            break;
        case LOG_NORMAL:
            fprintf(log_fp, "NORMAL ");
            break;
        case LOG_VERBOSE:
            fprintf(log_fp, "VERBOSE ");
            break;
        case LOG_DEBUG:
            fprintf(log_fp, "DEBUG ");
            break;
        case LOG_DEBUG_CRAZY:
            fprintf(log_fp, "CRAZY ");
            break;
    }
    fprintf(log_fp, "%s.%d] ", func, line);

    va_start(args, fmt);
    vfprintf(log_fp, fmt, args);
    va_end(args);

    fputc('\n', log_fp);
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

void *__xmalloc(size_t size, const char *file, const char *func, size_t line) {
    void *t;

    if (0 == size)
        return NULL;

    t = malloc(size);
    if (NULL == t) {
        LOGE("%s:%s():%zu: malloc(%zu) failed: %s", file, func, line, size, strerror(errno));
        DIEOS("malloc failed: %s", strerror(errno));
    }

    return t;
}

void *__xrealloc(void *ptr, size_t size, const char *file, const char *func, size_t line) {
    void *t;

    t = realloc(ptr, size);
    if (NULL == t) {
        LOGE("%s:%s():%zu: realloc(%p, %zu) failed: %s", file, func, line, ptr, size, strerror(errno));
        DIEOS("realloc failed: %s", strerror(errno));
    }

    return t;
}

char *__xstrndup(const char *str, size_t size, const char *file, const char *func, size_t line) {
    char *t;

    t = __xmalloc(size + 1, file, func, line);
    strncpy(t, str, size);
    t[size] = '\0';

    return t;
}

int remove_slash(char *dest, const char *src) {
    int gotslash = 0, nslashes = 0;

    for (int i = 0, j = 0; i < PATH_MAX; i++) {
        if ('/' == src[i]) {
            if (gotslash) {
                ++nslashes;
                continue;
            }
            else
                gotslash = 1;
        }
        else
            gotslash = 0;
        dest[j++] = src[i];
        /* Remove trailing slash */
        if ('\0' == src[i]) {
            if ('/' == dest[j - 2]) {
                ++nslashes;
                dest[j - 2] = '\0';
            }
            break;
        }
    }
    if (nslashes)
        LOGD("Simplified path \"%s\" to \"%s\"", src, dest);
    return nslashes;
}

void shell_expand(char *dest, const char *src) {
    char command[32 + PATH_MAX] = "/bin/sh -c 'echo -n \"";
    strncat(command, src, PATH_MAX);
    strncat(command, "\"'", 2);
    FILE *bash = popen(command, "r");
    if (NULL == bash)
        DIESOFT("bug in popen call: %s", strerror(errno));

    int i = 0;
    while (!feof(bash))
        dest[i++] = fgetc(bash);
    dest[i-1] = '\0';
    pclose(bash);
    if (0 != strncmp(src, dest, PATH_MAX))
        LOGD("Expanded path \"%s\" to \"%s\" using /bin/sh", src, dest);
}

char *getcwd_pid(char *dest, size_t size, pid_t pid) {
    int n;
    char cwd[PATH_MAX];

    snprintf(cwd, PATH_MAX, "/proc/%i/cwd", pid);
    n = readlink(cwd, dest, size - 1);
    if (0 > n)
        return NULL;
    /* Readlink does NOT append a NULL byte to buf. */
    dest[n] = '\0';
    return dest;
}

char *resolve_path(const char *path, int resolve) {
    char *rpath;

    if (NULL == path)
        return NULL;

    if (!resolve)
        rpath = erealpath(path, NULL);
    else
        rpath = realpath(path, NULL);

    if (NULL == rpath && ENOENT == errno) {
        // File doesn't exist, check the parent directory.
        char *dirc, *dname;
        dirc = xstrndup(path, PATH_MAX);
        dname = dirname(dirc);
        LOGD("File `%s' doesn't exist, using directory `%s'", path, dname);

        if (!resolve)
            rpath = erealpath(dname, NULL);
        else
            rpath = realpath(dname, NULL);
        free(dirc);

        if (NULL == rpath) {
            LOGD("Directory doesn't exist as well, setting errno to ENOENT");
            errno = ENOENT;
            return NULL;
        }
        else {
            char *basec, *bname;
            LOGD("File `%s' doesn't exist but directory `%s' exists, adding basename", path, rpath);
            basec = xstrndup(path, PATH_MAX);
            bname = basename(basec);
            strncat(rpath, "/", 1);
            strncat(rpath, bname, strlen(bname));
            free(basec);
        }
    }
    return rpath;
}

// Handle the ESRCH errno which means child is dead
int handle_esrch(context_t *ctx, struct tchild *child) {
    int ret = 0;
    if (ctx->eldest == child)
        ret = EX_SOFTWARE;
    tchild_delete(&(ctx->children), child->pid);
    return ret;
}
