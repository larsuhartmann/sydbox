/* vim: set sw=4 sts=4 fdm=syntax et : */

/**
 * Copyright (c) 2009 Ali Polatel
 * Copyright 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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

#ifndef __WRAPPERS_H__
#define __WRAPPERS_H__

#include <glib.h>

typedef enum canonicalize_mode_t
{
    CAN_EXISTING,       /* All components must exist.  */
    CAN_ALL_BUT_LAST,   /* All components excluding last one must exist.  */
} canonicalize_mode_t;

gchar *
edirname (const gchar *path);

gchar *
ebasename (const gchar *path);

gchar *
ereadlink (const gchar *path);

gchar *
egetcwd (void);

int
echdir (gchar *dir);

gchar *
canonicalize_filename_mode (const gchar *name,
                            canonicalize_mode_t can_mode,
                            gboolean resolve,
                            const gchar *cwd);

#endif

