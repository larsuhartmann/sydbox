/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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


#ifndef __SYDBOX_CONFIG_H__
#define __SYDBOX_CONFIG_H__

#include <glib.h>

gboolean
sydbox_config_load (const gchar * const config);

void
sydbox_config_update_from_environment (void);

void
sydbox_config_write_to_stderr (void);

const gchar *
sydbox_config_get_log_file (void);

gint
sydbox_config_get_verbosity (void);

void
sydbox_config_set_verbosity (gint verbosity);

gboolean
sydbox_config_get_sandbox_network (void);

const GSList *
sydbox_config_get_write_prefixes (void);

const GSList *
sydbox_config_get_predict_prefixes (void);

#endif

