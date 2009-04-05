/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __UTIL_H__
#define __UTIL_H__

#include "context.h"
#include "children.h"

#define NORMAL  "[00;00m"
#define MAGENTA "[00;35m"
#define PINK    "[01;35m"

extern int colour;

void
_die (int err, const char *fmt, ...)
    __attribute__ ((noreturn))
    __attribute__ ((format (printf, 2, 3)));

void
access_error (pid_t pid, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

char *
remove_slash (const char *src);

int
handle_esrch (context_t *ctx, struct tchild *child);

#endif

