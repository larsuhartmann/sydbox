/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
 * Based in part upon coreutils' lib/canonicalize.c which is:
 *  Copyright (C) 1996-2008 Free Software Foundation, Inc.
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

/* The following copyright pertains to egetcwd, echdir */
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgen.h>
#if defined(basename)
#undef basename // get the GNU version from string.h
#endif
#include <string.h>

#include <stddef.h>
#include <sys/stat.h>

#include <glib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "path.h"
#include "wrappers.h"

#ifndef __set_errno
# define __set_errno(v) errno = (v)
#endif

#ifndef MAXSYMLINKS
# define MAXSYMLINKS 256
#endif

// dirname wrapper which doesn't modify its argument
gchar *
edirname (const gchar *path)
{
    char *pathc = g_strdup (path);
    char *dname = dirname(pathc);
    char *dnamec = g_strdup (dname);
    g_free (pathc);
    return dnamec;
}

gchar *
ebasename (const gchar *path)
{
    return basename(path);
}

// readlink that allocates the string itself and appends a zero at the end
gchar *
ereadlink (const gchar *path)
{
    char *buf;
    long nrequested, nwritten;

    buf = NULL;
    nrequested = 32;
    for (;;) {
        buf = g_realloc (buf, nrequested);
        nwritten = readlink(path, buf, nrequested);
        if (G_UNLIKELY(0 > nwritten)) {
            g_free (buf);
            return NULL;
        }
        else if (nrequested > nwritten)
            break;
        else
            nrequested *= 2;
    }
    buf[nwritten] = '\0';
    return buf;
}


/* chdir with arbitrary long pathname.  Returns 0 on success, -1 on normal *
 * failure and -2 when chdir failed and the current directory is lost.  */

int
echdir (gchar *dir)
{
    char *s;
    int currdir = -2;

    for (;;) {
        if (!*dir || chdir(dir) == 0) {
#ifdef HAVE_FCHDIR
            if (currdir >= 0)
                close(currdir);
#endif
            return 0;
        }
        if ((errno != ENAMETOOLONG && errno != ENOMEM) ||
                strlen(dir) < PATH_MAX)
            break;
        for (s = dir + PATH_MAX - 1; s > dir && *s != '/'; s--)
            ;
        if (s == dir)
            break;
#ifdef HAVE_FCHDIR
        if (currdir == -2)
            currdir = open(".", O_RDONLY|O_NOCTTY);
#endif
        *s = '\0';
        if (chdir(dir) < 0) {
            *s = '/';
            break;
        }
#ifndef HAVE_FCHDIR
        currdir = -1;
#endif
        *s = '/';
        while (*++s == '/')
            ;
        dir = s;
    }
#ifdef HAVE_FCHDIR
    if (currdir >= 0) {
        if (fchdir(currdir) < 0) {
            close(currdir);
            return -2;
        }
        close(currdir);
        return -1;
    }
#endif
    return currdir == -2 ? -1 : -2;
}

gchar *
egetcwd (void)
{
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

#ifdef HAVE_GETCWD_NULL
    /* First try getcwd() */
    buf = getcwd(NULL, 0);
    if (NULL != buf)
        return buf;
    else if (ENAMETOOLONG != errno)
        return NULL;
#endif /* HAVE_GETCWD_NULL */

    /* Next try stat()'ing and chdir()'ing up */
    bufsiz = PATH_MAX;
    buf = g_malloc0 (bufsiz);
    pos = bufsiz - 1;
    buf[pos] = '\0';
    strcpy(nbuf, "../");
    if (0 > stat(".", &sbuf)) {
        g_free (buf);
        return NULL;
    }

    /* Record the initial inode and device */
    pino = sbuf.st_ino;
    pdev = sbuf.st_dev;

    for (;;) {
        if (0 > stat("..", &sbuf))
            break;

        /* Inode and device of current directory */
        ino = pino;
        dev = pdev;
        /* Inode and device of current directory's parent */
        pino = sbuf.st_ino;
        pdev = sbuf.st_dev;

        /* If they're the same, we've reached the root directory. */
        if (ino == pino && dev == pdev) {
            if (!buf[pos])
                buf[--pos] = '/';
            char *s = g_strdup (buf + pos);
            g_free (buf);
            echdir(s);
            return s;
        }

        /* Search the parent for the current directory. */
        dir = opendir("..");
        if (NULL == dir) {
            save_errno = errno;
            g_debug ("opendir() failed: %s", strerror(errno));
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
                /* Maybe found directory, need to check device & inode */
                strncpy(nbuf + 3, fn, PATH_MAX);
                lstat(nbuf, &sbuf);
                if (sbuf.st_dev == dev && sbuf.st_ino == ino)
                    break;
            }
        }
        closedir(dir);
        if (!de)
            break; /* Not found */
        len = strlen(nbuf + 2);
        pos -= len;
        while (pos <= 1) {
            char *temp;
            char *newbuf = g_malloc0 (2 * bufsiz);
            memcpy(newbuf + bufsiz, buf, bufsiz);
            temp = buf;
            buf = newbuf;
            g_free (temp);
            pos += bufsiz;
            bufsiz *= 2;
        }
        memcpy(buf + pos, nbuf + 2, len);

        if (0 > chdir(".."))
            break;
    }

    if (*buf) {
        g_debug ("changing current working directory to `%s'", buf + pos + 1);
        echdir(buf + pos + 1);
    }
    g_free (buf);
    return NULL;
}


/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated file name
   separators ('/') or symlinks.  Whether components must exist
   or not depends on canonicalize mode.  The result is malloc'd.  */

gchar *
canonicalize_filename_mode (const gchar *name,
                            canonicalize_mode_t can_mode,
                            gboolean resolve,
                            const gchar *cwd)
{
    int readlinks = 0;
    char *rname, *dest, *extra_buf = NULL;
    char const *start;
    char const *end;
    char const *rname_limit;
    size_t extra_len = 0;

    if (name == NULL) {
        __set_errno(EINVAL);
        return NULL;
    }

    if (name[0] == '\0') {
        __set_errno(ENOENT);
        return NULL;
    }

    if (name[0] != '/') {
        rname = g_strdup (cwd);
        dest = strchr(rname, '\0');
        if (dest - rname < PATH_MAX) {
            char *p = g_realloc (rname, PATH_MAX);
            dest = p + (dest - rname);
            rname = p;
            rname_limit = rname + PATH_MAX;
        }
        else
            rname_limit = dest;
    }
    else {
        cwd = NULL;
        rname = g_malloc (PATH_MAX);
        rname_limit = rname + PATH_MAX;
        rname[0] = '/';
        dest = rname + 1;
    }

    for (start = end = name; *start; start = end) {
        /* Skip sequence of multiple file name separators.  */
        while (*start == '/')
            ++start;

        /* Find end of component.  */
        for (end = start; *end && *end != '/'; ++end)
            /* Nothing.  */;

        if (end - start == 0)
            break;
        else if (end - start == 1 && start[0] == '.')
            /* nothing */;
        else if (end - start == 2 && start[0] == '.' && start[1] == '.') {
            /* Back up to previous component, ignore if at root already.  */
            if (dest > rname + 1)
                while ((--dest)[-1] != '/');
        }
        else {
            struct stat st;

            if (dest[-1] != '/')
                *dest++ = '/';

            if (dest + (end - start) >= rname_limit) {
                ptrdiff_t dest_offset = dest - rname;
                size_t new_size = rname_limit - rname;

                if (end - start + 1 > PATH_MAX)
                    new_size += end - start + 1;
                else
                    new_size += PATH_MAX;
                rname = g_realloc (rname, new_size);
                rname_limit = rname + new_size;

                dest = rname + dest_offset;
            }

            dest = memcpy (dest, start, end - start);
            dest += end - start;
            *dest = '\0';

            if (lstat (rname, &st) != 0) {
                if (can_mode == CAN_EXISTING)
                    goto error;
                if (can_mode == CAN_ALL_BUT_LAST && *end)
                    goto error;
                st.st_mode = 0;
            }

            if (S_ISLNK (st.st_mode)) {
                char *buf;
                size_t n, len;

                if (!resolve)
                    continue;

                /* Protect against infinite loops */
                if (readlinks++ > MAXSYMLINKS) {
                    __set_errno(ELOOP);
                    goto error;
                }

                buf = ereadlink(rname);
                if (!buf)
                    goto error;

                n = strlen (buf);
                len = strlen (end);

                if (!extra_len) {
                    extra_len =
                        ((n + len + 1) > PATH_MAX) ? (n + len + 1) : PATH_MAX;
                    extra_buf = g_malloc (extra_len);
                }
                else if ((n + len + 1) > extra_len) {
                    extra_len = n + len + 1;
                    extra_buf = g_realloc (extra_buf, extra_len);
                }

                /* Careful here, end may be a pointer into extra_buf... */
                memmove (&extra_buf[n], end, len + 1);
                name = end = memcpy (extra_buf, buf, n);

                if (buf[0] == '/')
                    dest = rname + 1;   /* It's an absolute symlink */
                else
                    /* Back up to previous component, ignore if at root already: */
                    if (dest > rname + 1)
                        while ((--dest)[-1] != '/');

                g_free (buf);
            }
            else {
                if (!S_ISDIR (st.st_mode) && *end) {
                    __set_errno(ENOTDIR);
                    goto error;
                }
            }
        }
    }
    if (dest > rname + 1 && dest[-1] == '/')
        --dest;
    *dest = '\0';

    g_free (extra_buf);
    return rname;

error:
  g_free (extra_buf);
  g_free (rname);
  return NULL;
}

