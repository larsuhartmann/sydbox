/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
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

#include <stdbool.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "path.h"
#include "sydbox-log.h"
#include "sydbox-config.h"

struct sydbox_config
{
    gchar *logfile;

    gint verbosity;

    bool sandbox_path;
    bool sandbox_exec;
    bool sandbox_network;
    bool colourise_output;
    bool disallow_magic_commands;
    bool paranoid_mode_enabled;
    bool wait_all;
    bool allow_proc_pid;

    GSList *write_prefixes;
    GSList *predict_prefixes;
    GSList *exec_prefixes;
} *config;


bool
sydbox_config_load (const gchar * const file)
{
    const gchar *config_file;
    GKeyFile *config_fd;
    GError *config_error = NULL;

    g_return_val_if_fail(!config, true);

    // Initialize config structure
    config = g_new0 (struct sydbox_config, 1);

    if (g_getenv(ENV_NO_CONFIG)) {
        /* ENV_NO_CONFIG set, set the defaults,
         * and return without parsing the configuration file.
         */
        config->colourise_output = true;
        config->verbosity = 1;
        config->sandbox_path = true;
        config->sandbox_network = false;
        config->sandbox_exec = false;
        config->disallow_magic_commands = false;
        config->paranoid_mode_enabled = false;
        config->wait_all = false;
        config->allow_proc_pid = true;
        return true;
    }

    // Figure out the path to the configuration file
    if (file)
        config_file = file;
    else if (g_getenv(ENV_CONFIG))
        config_file = g_getenv(ENV_CONFIG);
    else
        config_file = SYSCONFDIR G_DIR_SEPARATOR_S "sydbox.conf";

    // Initialize key file
    config_fd = g_key_file_new();
    if (!g_key_file_load_from_file(config_fd, config_file, G_KEY_FILE_NONE, &config_error)) {
        g_printerr("failed to parse config file: %s\n", config_error->message);
        g_error_free(config_error);
        g_key_file_free(config_fd);
        g_free(config);
        return false;
    }

    // Get main.colour
    if (g_getenv(ENV_NO_COLOUR))
        config->colourise_output = false;
    else {
        config->colourise_output = g_key_file_get_boolean(config_fd, "main", "colour", &config_error);
        if (!config->colourise_output) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("main.colour not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->colourise_output = true;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get main.paranoid
    config->paranoid_mode_enabled = g_key_file_get_boolean(config_fd, "main", "paranoid", &config_error);
    if (!config->paranoid_mode_enabled && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.paranoid not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->paranoid_mode_enabled = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get main.lock
    if (g_getenv(ENV_LOCK))
        config->disallow_magic_commands = true;
    else {
        config->disallow_magic_commands = g_key_file_get_boolean(config_fd, "main", "lock", &config_error);
        if (!config->disallow_magic_commands && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("main.lock not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->disallow_magic_commands = false;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get main.wait_all
    if (g_getenv(ENV_WAIT_ALL))
        config->wait_all = true;
    else {
        config->wait_all = g_key_file_get_boolean(config_fd, "main", "wait_all", &config_error);
        if (!config->wait_all && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("main.wait_all not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->wait_all = false;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get main.allow_proc_pid
    config->allow_proc_pid = g_key_file_get_boolean(config_fd, "main", "allow_proc_pid", &config_error);
    if (!config->allow_proc_pid && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.allow_proc_pid not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->allow_proc_pid = true;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get log.file
    if (g_getenv(ENV_LOG))
        config->logfile = g_strdup(g_getenv(ENV_LOG));
    else
        config->logfile = g_key_file_get_string(config_fd, "log", "file", NULL);

    // Get log.level
    config->verbosity = g_key_file_get_integer(config_fd, "log", "level", &config_error);
    if (0 == config->verbosity) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("log.level not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return FALSE;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->verbosity = 1;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get sandbox.path
    if (g_getenv(ENV_DISABLE_PATH))
        config->sandbox_path = false;
    else {
        config->sandbox_path = g_key_file_get_boolean(config_fd, "sandbox", "path", &config_error);
        if (!config->sandbox_path && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("sandbox.path not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->sandbox_path = true;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get sandbox.exec
    if (g_getenv(ENV_EXEC))
        config->sandbox_exec = true;
    else {
        config->sandbox_exec = g_key_file_get_boolean(config_fd, "sandbox", "exec", &config_error);
        if (!config->sandbox_exec && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("sandbox.exec not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->sandbox_exec = false;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get sandbox.net
    if (g_getenv(ENV_NET))
        config->sandbox_network = true;
    else {
        config->sandbox_network = g_key_file_get_boolean(config_fd, "sandbox", "network", &config_error);
        if (!config->sandbox_network && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("sandbox.network not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return false;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->sandbox_network = false;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }


    // Get prefix.write
    char **write_prefixes = g_key_file_get_string_list(config_fd, "prefix", "write", NULL, NULL);
    if (NULL != write_prefixes) {
        for (unsigned int i = 0; NULL != write_prefixes[i]; i++)
            pathnode_new_early(&config->write_prefixes, write_prefixes[i], 1);
        g_strfreev(write_prefixes);
    }

    // Get prefix.predict
    char **predict_prefixes = g_key_file_get_string_list(config_fd, "prefix", "predict", NULL, NULL);
    if (NULL != predict_prefixes) {
        for (unsigned int i = 0; NULL != predict_prefixes[i]; i++)
            pathnode_new_early(&config->predict_prefixes, predict_prefixes[i], 1);
        g_strfreev(predict_prefixes);
    }

    // Get prefix.exec
    char **exec_prefixes = g_key_file_get_string_list(config_fd, "prefix", "exec", NULL, NULL);
    if (NULL != exec_prefixes) {
        for (unsigned int i = 0; NULL != exec_prefixes[i]; i++)
            pathnode_new_early(&config->exec_prefixes, exec_prefixes[i], 1);
        g_strfreev(exec_prefixes);
    }

    // Cleanup and return
    g_key_file_free(config_fd);
    return true;
}

void
sydbox_config_update_from_environment (void)
{
    g_info ("extending path list using environment variable " ENV_WRITE);
    pathlist_init (&config->write_prefixes, g_getenv (ENV_WRITE));

    g_info ("extending path list using environment variable " ENV_PREDICT);
    pathlist_init (&config->predict_prefixes, g_getenv (ENV_PREDICT));

    g_info ("extending path list using environment variable " ENV_EXEC_ALLOW);
    pathlist_init(&config->exec_prefixes, g_getenv(ENV_EXEC_ALLOW));
}


static inline void
print_slist_entry (gpointer data, gpointer userdata G_GNUC_UNUSED)
{
    gchar *cdata = (gchar *) data;
    if (NULL != cdata && '\0' != cdata[0])
        g_fprintf (stderr, "\t%s\n", cdata);
}

void
sydbox_config_write_to_stderr (void)
{
    g_fprintf (stderr, "main.colour = %s\n", config->colourise_output ? "on" : "off");
    g_fprintf (stderr, "main.lock = %s\n", config->disallow_magic_commands ? "set" : "unset");
    g_fprintf (stderr, "main.paranoid = %s\n", config->paranoid_mode_enabled ? "yes" : "no");
    g_fprintf (stderr, "main.wait_all = %s\n", config->wait_all ? "yes" : "no");
    g_fprintf (stderr, "main.allow_proc_pid = %s\n", config->allow_proc_pid ? "yes" : "no");
    g_fprintf (stderr, "log.file = %s\n", config->logfile ? config->logfile : "stderr");
    g_fprintf (stderr, "log.level = %d\n", config->verbosity);
    g_fprintf (stderr, "sandbox.path = %s\n", config->sandbox_path ? "yes" : "no");
    g_fprintf (stderr, "sandbox.exec = %s\n", config->sandbox_exec ? "yes" : "no");
    g_fprintf (stderr, "sandbox.network = %s\n", config->sandbox_network ? "yes" : "no");
    g_fprintf (stderr, "prefix.write:\n");
    g_slist_foreach (config->write_prefixes, print_slist_entry, NULL);
    g_fprintf (stderr, "prefix.predict:\n");
    g_slist_foreach (config->predict_prefixes, print_slist_entry, NULL);
    g_fprintf (stderr, "prefix.exec\n");
    g_slist_foreach (config->exec_prefixes, print_slist_entry, NULL);
}


const gchar *
sydbox_config_get_log_file (void)
{
    return config->logfile;
}

void
sydbox_config_set_log_file (const gchar * const logfile)
{
    if (config->logfile)
        g_free (config->logfile);

    config->logfile = g_strdup (logfile);
}

gint
sydbox_config_get_verbosity (void)
{
    return config->verbosity;
}

void
sydbox_config_set_verbosity (gint verbosity)
{
    config->verbosity = verbosity;
}

bool
sydbox_config_get_sandbox_path (void)
{
    return config->sandbox_path;
}

void
sydbox_config_set_sandbox_path (bool on)
{
    config->sandbox_path = on;
}

bool
sydbox_config_get_sandbox_exec (void)
{
    return config->sandbox_exec;
}

void
sydbox_config_set_sandbox_exec (bool on)
{
    config->sandbox_exec = on;
}

bool
sydbox_config_get_sandbox_network (void)
{
    return config->sandbox_network;
}

void
sydbox_config_set_sandbox_network (bool on)
{
    config->sandbox_network = on;
}

bool
sydbox_config_get_colourise_output (void)
{
    return config->colourise_output;
}

void
sydbox_config_set_colourise_output (bool colourise)
{
    config->colourise_output = colourise;
}

bool
sydbox_config_get_disallow_magic_commands (void)
{
    return config->disallow_magic_commands;
}

void
sydbox_config_set_disallow_magic_commands (bool disallow)
{
    config->disallow_magic_commands = disallow;
}

bool
sydbox_config_get_wait_all (void)
{
    return config->wait_all;
}

void
sydbox_config_set_wait_all (bool waitall)
{
    config->wait_all = waitall;
}

bool
sydbox_config_get_allow_proc_pid (void)
{
    return config->allow_proc_pid;
}

void
sydbox_config_set_allow_proc_pid (bool allow)
{
    config->allow_proc_pid = allow;
}

bool
sydbox_config_get_paranoid_mode_enabled (void)
{
    return config->paranoid_mode_enabled;
}

void
sydbox_config_set_paranoid_mode_enabled (bool enabled)
{
    config->paranoid_mode_enabled = enabled;
}

GSList *
sydbox_config_get_write_prefixes (void)
{
    return config->write_prefixes;
}

GSList *
sydbox_config_get_predict_prefixes (void)
{
    return config->predict_prefixes;
}

GSList *
sydbox_config_get_exec_prefixes (void)
{
    return config->exec_prefixes;
}

