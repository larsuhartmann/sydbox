/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __PROC_H__
#define __PROC_H__

#include "defs.h"

char *
pgetcwd (context_t *ctx, pid_t pid);

char *
pgetdir (context_t *ctx, pid_t pid, int dfd);

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

