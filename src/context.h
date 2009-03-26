/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "children.h"

typedef struct
{
    int paranoid;
    char *cwd;                // current working directory
    struct tchild *children;
    struct tchild *eldest;    // first child is kept to determine return code
} context_t;

context_t *
context_new (void);

void
context_free (context_t *ctx);

#endif

