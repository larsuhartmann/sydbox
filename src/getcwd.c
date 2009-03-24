/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 1992-1997 Paul Falstad
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and to distribute modified versions of this software for any
 * purpose, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * In no event shall Paul Falstad or the Zsh Development Group be liable
 * to any party for direct, indirect, special, incidental, or consequential
 * damages arising out of the use of this software and its documentation,
 * even if Paul Falstad and the Zsh Development Group have been advised of
 * the possibility of such damage.
 *
 * Paul Falstad and the Zsh Development Group specifically disclaim any
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.  The software
 * provided hereunder is on an "as is" basis, and Paul Falstad and the
 * Zsh Development Group have no obligation to provide maintenance,
 * support, updates, enhancements, or modifications.
 *
 */

#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"

/* chdir with arbitrary long pathname.  Returns 0 on success, -1 on normal *
 * failure and -2 when chdir failed and the current directory is lost.  */

int echdir(char *dir) {
    char *s;
    int currdir = -2;

    for (;;) {
        if (!*dir || chdir(dir) == 0) {
            return 0;
        }
        if ((errno != ENAMETOOLONG && errno != ENOMEM) ||
                strlen(dir) < PATH_MAX)
            break;
        for (s = dir + PATH_MAX - 1; s > dir && *s != '/'; s--)
            ;
        if (s == dir)
            break;
        *s = '\0';
        if (chdir(dir) < 0) {
            *s = '/';
            break;
        }
        *s = '/';
        while (*++s == '/')
            ;
        dir = s;
    }
    return currdir == -2 ? -1 : -2;
}

char *egetcwd(void) {
    char nbuf[PATH_MAX+3];
    char *buf;
    int bufsiz, pos;
    struct stat sbuf;
    ino_t pino;
    dev_t pdev;
    struct dirent *de;
    DIR *dir;
    dev_t dev;
    ino_t ino;
    int len;
    int save_errno;

    bufsiz = PATH_MAX;
    buf = xcalloc(bufsiz, sizeof(char));
    pos = bufsiz - 1;
    buf[pos] = '\0';
    strcpy(nbuf, "../");
    if (0 > stat(".", &sbuf)) {
        free(buf);
        return NULL;
    }

    pino = sbuf.st_ino;
    pdev = sbuf.st_dev;

    for (;;) {
        if (0 > stat("..", &sbuf))
            break;

        ino = pino;
        dev = pdev;
        pino = sbuf.st_ino;
        pdev = sbuf.st_dev;

        if (ino == pino && dev == pdev) {
            if (!buf[pos])
                buf[--pos] = '/';
            char *s = xstrdup(buf + pos);
            free(buf);
            echdir(s);
            return s;
        }

        dir = opendir("..");
        if (NULL == dir) {
            save_errno = errno;
            LOGD("opendir() failed: %s", strerror(errno));
            errno = save_errno;
            break;
        }

        while ((de = readdir(dir))) {
            char *fn = de->d_name;
            /* Ignore `.' and `..'. */
            if (fn[0] == '.' &&
                (fn[1] == '\0' ||
                 (fn[1] == '.' && fn[2] == '\0')))
                continue;
            if (dev != pdev || (ino_t) de->d_ino == ino) {
                strncpy(nbuf + 3, fn, PATH_MAX);
                lstat(nbuf, &sbuf);
                if (sbuf.st_dev == dev && sbuf.st_ino == ino)
                    break;
            }
        }
        closedir(dir);
        if (!de)
            break;
        len = strlen(nbuf + 2);
        pos -= len;
        while (pos <= 1) {
            char *temp;
            char *newbuf = xcalloc(2 * bufsiz, sizeof(char));
            memcpy(newbuf + bufsiz, buf, bufsiz);
            temp = buf;
            buf = newbuf;
            free(temp);
            pos += bufsiz;
            bufsiz *= 2;
        }
        memcpy(buf + pos, nbuf + 2, len);

        if (0 > chdir(".."))
            break;
    }

    if (*buf) {
        LOGD("Changing current working directory to `%s'", buf);
        echdir(buf + pos + 1);
    }
    free(buf);
    return NULL;
}
