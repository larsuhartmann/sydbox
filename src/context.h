/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 **/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "children.h"

typedef struct
{
    int paranoid;
    char *cwd;              // current working directory
    struct tchild *eldest;  // first child is kept to determine return code
    GSList *children;       // list of children
} context_t;

context_t *
context_new (void);

void
context_free (context_t *ctx);

#endif /* __CONTEXT_H__ */

