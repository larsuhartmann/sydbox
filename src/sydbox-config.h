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
#define ENV_LOG             "SYDBOX_LOG"
#define ENV_CONFIG          "SYDBOX_CONFIG"
#define ENV_WRITE           "SYDBOX_WRITE"
#define ENV_PREDICT         "SYDBOX_PREDICT"
#define ENV_EXEC_ALLOW      "SYDBOX_EXEC_ALLOW"
#define ENV_DISABLE_PATH    "SYDBOX_DISABLE_PATH"
#define ENV_EXEC            "SYDBOX_EXEC"
#define ENV_NET             "SYDBOX_NET"
#define ENV_NO_COLOUR       "SYDBOX_NO_COLOUR"
#define ENV_NO_CONFIG       "SYDBOX_NO_CONFIG"
#define ENV_LOCK            "SYDBOX_LOCK"
#define ENV_WAIT_ALL        "SYDBOX_WAIT_ALL"

/**
 * sydbox_config_load:
 * @param config: path to the configuration file.
 *
 * Loads the configuration from the file specified by @config.  If @config is
 * %NULL, the environment variable %SYDBOX_CONFIG is consulted.  If
 * %SYDBOX_CONFIG is also unset, the default configuration file is consulte.
 * Returns %TRUE if the operation succeeds, %FALSE otherwise.
 *
 * Returns: a #gboolean indicating if the config file was loaded successfully
 *
 * Since: 0.1_alpha
 **/
bool
sydbox_config_load (const gchar * const config);

/**
 * sydbox_config_update_from_environment:
 *
 * Update the configuration state from supported environment variables.
 *
 * Since: 0.1_alpha
 **/
void
sydbox_config_update_from_environment (void);

/**
 * sydbox_config_write_to_stderr:
 *
 * Writes the configuration state to stderr.
 *
 * Since: 0.1_alpha
 **/
void
sydbox_config_write_to_stderr (void);

/**
 * sydbox_config_get_log_file:
 *
 * Accessor for the log file being used for the current state.
 *
 * Returns: the path to the logfile
 *
 * Since: 0.1_alpha
 **/
const gchar *
sydbox_config_get_log_file (void);

/**
 * sydbox_config_set_log_file:
 * @logfile: path to the logfile
 *
 * Sets the logfile that should be used.
 *
 * Since: 0.1_alpha
 **/
void
sydbox_config_set_log_file (const gchar * const logfile);

/**
 * sydbox_config_get_verbosity:
 *
 * Accessor for the current verbosity level
 *
 * Returns: a #gint indicating the current log level
 *
 * Since: 0.1_alpha
 **/
gint
sydbox_config_get_verbosity (void);

/**
 * sydbox_config_set_verbosity:
 * @verbosity: #gint specifying the current log level
 *
 * Sets the current loglevel to be used
 *
 * Since: 0.1_alpha
 **/
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

/**
 * sydbox_config_get_sandbox_network:
 *
 * Returns %TRUE if network accesses should be sandboxed, %FALSE otherwise
 *
 * Returns: #gboolean indicating if network accesses should be sandboxed
 *
 * Since: 0.1_alpha
 **/
bool
sydbox_config_get_sandbox_network (void);

void
sydbox_config_set_sandbox_network (bool on);

/**
 * sydbox_config_get_colourise_output:
 *
 * Returns %TRUE if the output should be coloured, %FALSE otherwise
 *
 * Returns: a #gboolean stating if output should have colour
 *
 * Since: 0.1_alpha
 **/
bool
sydbox_config_get_colourise_output (void);

/**
 * sydbox_config_set_colourise_output:
 * @colourise: a #gboolean indicating if output should be coloured
 *
 * Sets whether the output should be coloured
 *
 * Since: 0.1_alpha
 **/
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

/**
 * sydbox_config_get_paranoid_mode_enabled:
 *
 * Returns %TRUE if paranoid mode is enabled, %FALSE otherwise
 *
 * Returns: a #gboolean indicating if paranoid mode is enabled
 *
 * Since: 0.1_alpha
 **/
bool
sydbox_config_get_paranoid_mode_enabled (void);

/**
 * sydbox_config_set_paranoid_mode_enabled:
 * @enabled: a #gboolean indicating if paranoid mode is enabled
 *
 * Sets whether paranoid mode is enabled
 *
 * Since: 0.1_alpha
 **/
void
sydbox_config_set_paranoid_mode_enabled (bool enabled);

/**
 * sydbox_config_get_write_prefixes:
 *
 * Returns a list of permitted write prefixes
 *
 * Returns: a #GSList containing permitted write prefixes
 *
 * Since: 0.1_alpha
 **/
GSList *
sydbox_config_get_write_prefixes (void);

/**
 * sydbox_config_get_predict_prefixes:
 *
 * Returns a list of predicted write prefixes
 *
 * Returns: a #GSList containing predicted write prefixes
 *
 * Since: 0.1_alpha
 **/
GSList *
sydbox_config_get_predict_prefixes (void);

GSList *
sydbox_config_get_exec_prefixes (void);

void sydbox_config_addhook(gchar *path);

int sydbox_config_rmhook(const gchar *path);

void sydbox_config_rmhook_all(void);

GSList *sydbox_config_get_hooks(void);

#endif

