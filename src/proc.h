/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __PROC_H__
#define __PROC_H__

#include "context.h"

char *
pgetcwd (pid_t pid);

char *
pgetdir (pid_t pid, int dfd);

#endif

