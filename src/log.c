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

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "log.h"
#include "defs.h"
#include "util.h"

int log_level = -1;
char *log_file = NULL;
FILE *log_fp = NULL;

void
lg(int level, const char *func, size_t line, const char *fmt, ...)
{
    static int log_file_opened = 0;
    va_list args;

    if (!log_file_opened) {
        int isstderr = NULL == log_file ? 1 : 0;

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
    fprintf(log_fp, "%s.%zu] ", func, line);

    va_start(args, fmt);
    vfprintf(log_fp, fmt, args);
    va_end(args);

    fputc('\n', log_fp);
}

