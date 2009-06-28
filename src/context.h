/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
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
 **/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdbool.h>
#include <sys/types.h>

#include "children.h"

typedef struct
{
    pid_t eldest;               // first child's pid is kept to determine return code.
    bool before_initial_execve; // first execve() is noted here for execve(2) sandboxing.
    GSList *children;           // list of children
} context_t;

context_t *
context_new (void);

void
context_free (context_t *ctx);

void
context_remove_child (context_t * const ctx, pid_t pid);

#endif /* __CONTEXT_H__ */

