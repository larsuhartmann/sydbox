/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
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
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "proc.h"
#include "wrappers.h"

char *pgetcwd(context_t *ctx, pid_t pid) {
    int ret;
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
    ret = echdir(linkcwd);
    if (0 == ret || -2 == ret) {
        /* Either we've chdir()'ed successfully or current working directory was
         * lost during the chdir() attempt so we need to update ctx->cwd.
         */
        g_free(ctx->cwd);
        ctx->cwd = egetcwd();
        if (NULL == ctx->cwd) {
            g_printerr("failed to get current working directory: %s", g_strerror(errno));
            exit(-1);
        }
        if (0 == ret) {
            /* echdir() was successful */
            return g_strdup(ctx->cwd);
        }
    }
    return NULL;
}

char *pgetdir(context_t *ctx, pid_t pid, int dfd) {
    int ret;
    char *dir;
    char linkdir[128];
    snprintf(linkdir, 128, "/proc/%i/fd/%d", pid, dfd);

    // First try ereadlink()
    dir = ereadlink(linkdir);
    if (NULL != dir)
        return dir;
    else if (ENAMETOOLONG != errno)
        return NULL;

    // Now try egetcwd()
    errno = 0;
    ret = echdir(linkdir);
    if (0 == ret || -2 == ret) {
        /* Either we've chdir()'ed successfully or current working directory was
         * lost during the chdir() attempt so we need to update ctx->cwd.
         */
        g_free(ctx->cwd);
        ctx->cwd = egetcwd();
        if (NULL == ctx->cwd) {
            g_printerr("failed to get current working directory: %s", g_strerror(errno));
            exit(-1);
        }
        if (0 == ret) {
            /* echdir() was successful */
            return g_strdup(ctx->cwd);
        }
    }
    return NULL;
}
