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

// Environment variables
#define ENV_LOG             "SANDBOX_LOG"
#define ENV_CONFIG          "SANDBOX_CONFIG"
#define ENV_WRITE           "SANDBOX_WRITE"
#define ENV_PREDICT         "SANDBOX_PREDICT"
#define ENV_EXEC_ALLOW      "SANDBOX_EXEC_ALLOW"
#define ENV_DISABLE_PATH    "SANDBOX_DISABLE_PATH"
#define ENV_EXEC            "SANDBOX_EXEC"
#define ENV_NET             "SANDBOX_NET"
#define ENV_NO_COLOUR       "SANDBOX_NO_COLOUR"
#define ENV_NO_CONFIG       "SANDBOX_NO_CONFIG"
#define ENV_LOCK            "SANDBOX_LOCK"
#define ENV_WAIT_ALL        "SANDBOX_WAIT_ALL"

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
sydbox_config_get_sandbox_path (void);

void
sydbox_config_set_sandbox_path (bool on);

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
sydbox_config_get_wait_all (void);

void
sydbox_config_set_wait_all (bool waitall);

bool
sydbox_config_get_allow_proc_pid (void);

void
sydbox_config_set_allow_proc_pid (bool allow);

bool
sydbox_config_get_paranoid_mode_enabled (void);

void
sydbox_config_set_paranoid_mode_enabled (bool enabled);

GSList *
sydbox_config_get_write_prefixes (void);

GSList *
sydbox_config_get_predict_prefixes (void);

GSList *
sydbox_config_get_exec_prefixes (void);

#endif

