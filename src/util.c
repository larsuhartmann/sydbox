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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "log.h"
#include "util.h"
#include "config.h"
#include "children.h"

#include "sydbox-config.h"

void access_error(pid_t pid, const char *fmt, ...) {
    va_list args;
    time_t now;

    now = time(NULL);
    if (sydbox_config_get_colourise_output ()) {
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

    if (sydbox_config_get_colourise_output ())
        fprintf(stderr, NORMAL "\n");
    else
        fputc('\n', stderr);
}

char *remove_slash(const char *src) {
    int gotslash = 0, hasnonslash = 0, nslashes = 0;
    int len = strlen(src) + 1;
    char *dest = NULL;

    for (int i = 0, j = 0; i < len; i++) {
        if ('/' == src[i]) {
            if (gotslash) {
                ++nslashes;
                continue;
            }
            else
                gotslash = 1;
        }
        else {
            gotslash = 0;
            if ('\0' != src[i])
                hasnonslash = 1;
        }
        dest = g_realloc (dest, (++j + 1) * sizeof(char));
        dest[j-1] = src[i];
        /* Remove trailing slash */
        if (hasnonslash && '\0' == src[i]) {
            if ('/' == dest[j - 2]) {
                ++nslashes;
                dest[j - 2] = '\0';
            }
            break;
        }
    }
    if (nslashes)
        g_debug ("simplified path `%s' to `%s', removed %d slashes", src, dest, nslashes);
    return dest;
}

// Handle the ESRCH errno which means child is dead
int handle_esrch(context_t *ctx, struct tchild *child) {
    int ret = 0;
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "handling ESRCH for child %i", child->pid);
    if (ctx->eldest == child)
        ret = EX_SOFTWARE;
    tchild_delete(&(ctx->children), child->pid);
    return ret;
}
