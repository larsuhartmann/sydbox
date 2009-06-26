/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Saleem Abdulrasool <compnerd@compnerd.org>
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


#ifndef __SYDBOX_CONFIG_H__
#define __SYDBOX_CONFIG_H__

#include <stdbool.h>

#include <glib.h>

bool
sydbox_config_load (const gchar * const config);

void
sydbox_config_update_from_environment (void);

void
sydbox_config_write_to_stderr (void);

const gchar *
sydbox_config_get_log_file (void);

void
sydbox_config_set_log_file (const gchar * const logfile);

gint
sydbox_config_get_verbosity (void);

void
sydbox_config_set_verbosity (gint verbosity);

bool
sydbox_config_get_sandbox_exec (void);

void
sydbox_config_set_sandbox_exec (bool on);

bool
sydbox_config_get_sandbox_network (void);

void
sydbox_config_set_sandbox_network (bool on);

bool
sydbox_config_get_colourise_output (void);

void
sydbox_config_set_colourise_output (bool colourise);

bool
sydbox_config_get_disallow_magic_commands (void);

void
sydbox_config_set_disallow_magic_commands (bool disallow);

bool
sydbox_config_get_paranoid_mode_enabled (void);

void
sydbox_config_set_paranoid_mode_enabled (bool enabled);

const GSList *
sydbox_config_get_write_prefixes (void);

const GSList *
sydbox_config_get_predict_prefixes (void);

const GSList *
sydbox_config_get_exec_prefixes (void);

#endif

