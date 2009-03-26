/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 **/

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "context.h"
#include "children.h"

struct syscall_def
{
    int no;
    unsigned int flags;
};

enum res_syscall
{
    RS_DENY,                  // Deny access
    RS_ALLOW,                 // Allow access
    RS_NONMAGIC,              // open() or stat() not magic (internal)
    RS_ERROR = EX_SOFTWARE,   // An error occured while checking access
};

enum res_syscall
syscall_check (context_t *ctx, struct tchild *child, int sno);

int
syscall_handle (context_t *ctx, struct tchild *child);

#endif

/* vim: set sw=4 sts=4 fdm=syntax et : */

