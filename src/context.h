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

#include "children.h"

typedef struct
{
    char *cwd;              // current working directory
    struct tchild *eldest;  // first child is kept to determine return code
    GSList *children;       // list of children
} context_t;

context_t *
context_new (void);

void
context_free (context_t *ctx);

#endif /* __CONTEXT_H__ */
