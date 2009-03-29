/* vim: set sw=4 sts=4 fdm=syntax et : */

/*
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
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <glib.h>

gboolean
sydbox_log_init (const char * const filename,
                 const gint verbosity);

void
sydbox_log_fini (void);

#endif

