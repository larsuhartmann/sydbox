/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
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

#ifndef __SYDBOX_CONTEXT_INTERNAL_H__
#define __SYDBOX_CONTEXT_INTERNAL_H__

#include <sys/types.h>

#include <glib.h>

G_BEGIN_DECLS

struct _SydboxContext {
    gchar *cwd;         // current working directory
    pid_t eldest;       // first child's pid is kept to determine return code
    GSList *children;   // list of children

    gboolean network;
    GSList *predict_prefixes;
    GSList *write_prefixes;
};

G_END_DECLS

#endif // __SYDBOX_CONTEXT_INTERNAL_H__

