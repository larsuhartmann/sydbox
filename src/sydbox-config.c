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

#include <glib.h>
#include <glib/gstdio.h>

#include "path.h"
#include "sydbox-log.h"
#include "sydbox-config.h"

/* environment variables */
#define ENV_LOG         "SANDBOX_LOG"
#define ENV_CONFIG      "SANDBOX_CONFIG"
#define ENV_WRITE       "SANDBOX_WRITE"
#define ENV_PREDICT     "SANDBOX_PREDICT"
#define ENV_NET         "SANDBOX_NET"
#define ENV_NO_COLOUR   "SANDBOX_NO_COLOUR"
#define ENV_NO_CONFIG   "SANDBOX_NO_CONFIG"

struct sydbox_config
{
    gchar *logfile;

    gint verbosity;

    gboolean sandbox_network;
    gboolean colourise_output;
    gboolean allow_magic_commands;
    gboolean paranoid_mode_enabled;

    GSList *write_prefixes;
    GSList *predict_prefixes;
} *config;


gboolean
sydbox_config_load (const gchar * const file)
{
    gchar *config_file;
    GKeyFile *config_fd;
    GError *config_error = NULL;

    g_return_val_if_fail(!config, TRUE);

    // Figure out the path to the configuration file
    if (file)
        config_file = g_strdup (file);
    else if (g_getenv (ENV_CONFIG))
        config_file = g_strdup (g_getenv (ENV_CONFIG));
    else
        config_file = g_strdup (SYSCONFDIR G_DIR_SEPARATOR_S "sydbox.conf");

    // Initialize config structure
    config = g_new0 (struct sydbox_config, 1);

    if (g_getenv(ENV_NO_CONFIG)) {
        /* ENV_NO_CONFIG set, set the defaults and return without parsing the
         * configuration file.
         */
        config->colourise_output = TRUE;
        config->allow_magic_commands = TRUE;
        return TRUE;
    }

    // Initialize key file
    config_fd = g_key_file_new();
    if (!g_key_file_load_from_file(config_fd, config_file, G_KEY_FILE_NONE, &config_error)) {
        g_printerr("failed to parse config file: %s\n", config_error->message);
        g_error_free(config_error);
        g_key_file_free(config_fd);
        g_free(config);
        return FALSE;
    }

    // Get main.log_file
    if (g_getenv(ENV_LOG))
        config->logfile = g_strdup(g_getenv(ENV_LOG));
    else
        config->logfile = g_key_file_get_string(config_fd, "main", "log_file", NULL);

    // Get main.log_level
    config->verbosity = g_key_file_get_integer(config_fd, "main", "log_level", &config_error);
    if (0 == config->verbosity) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.log_level not an integer: %s", config_error->message);
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

    // Get main.colour
    if (g_getenv(ENV_NO_COLOUR))
        config->colourise_output = FALSE;
    else {
        config->colourise_output = g_key_file_get_boolean(config_fd, "main", "colour", &config_error);
        if (!config->colourise_output) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("main.colour not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return FALSE;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->colourise_output = TRUE;
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }
    }

    // Get main.net
    if (g_getenv(ENV_NET))
        config->sandbox_network = TRUE;
    else {
        config->sandbox_network = g_key_file_get_boolean(config_fd, "main", "net", &config_error);
        if (!config->sandbox_network && config_error) {
            switch (config_error->code) {
                case G_KEY_FILE_ERROR_INVALID_VALUE:
                    g_printerr("main.net not a boolean: %s", config_error->message);
                    g_error_free(config_error);
                    g_key_file_free(config_fd);
                    g_free(config);
                    return FALSE;
                case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                    g_error_free(config_error);
                    config_error = NULL;
                    config->sandbox_network = FALSE;
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
                return FALSE;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->paranoid_mode_enabled = FALSE;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get main.lock
    config->allow_magic_commands = g_key_file_get_boolean(config_fd, "main", "lock", &config_error);
    if (!config->allow_magic_commands && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.lock not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return FALSE;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->allow_magic_commands = TRUE;
                break;
            default:
                g_assert_not_reached();
                break;
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

    // Cleanup and return
    g_key_file_free(config_fd);
    g_free(config_file);
    return TRUE;
}

void
sydbox_config_update_from_environment (void)
{
    g_info ("extending path list using environment variable " ENV_WRITE);
    pathlist_init (&config->write_prefixes, g_getenv (ENV_WRITE));

    g_info ("extending path list using environment variable " ENV_PREDICT);
    pathlist_init (&config->predict_prefixes, g_getenv (ENV_PREDICT));
}


static inline void
print_slist_entry (gpointer data, gpointer userdata G_GNUC_UNUSED)
{
    g_fprintf (stderr, "\t%s\n", (gchar *) data);
}

void
sydbox_config_write_to_stderr (void)
{
    g_fprintf (stderr, "colour = %s\n", config->colourise_output ? "yes" : "no");
    g_fprintf (stderr, "lock = %s\n", config->allow_magic_commands ? "unset" : "set");
    g_fprintf (stderr, "log_file = %s\n", config->logfile ? config->logfile : "stderr");
    g_fprintf (stderr, "log_level = %d\n", config->verbosity);
    g_fprintf (stderr, "network sandboxing = %s\n", config->sandbox_network ? "yes" : "no");
    g_fprintf (stderr, "paranoid = %s\n", config->paranoid_mode_enabled ? "yes" : "no");
    g_fprintf (stderr, "allowed write prefixes:\n");
    g_slist_foreach (config->write_prefixes, print_slist_entry, NULL);
    g_fprintf (stderr, "predicted write prefixes:\n");
    g_slist_foreach (config->predict_prefixes, print_slist_entry, NULL);
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

gboolean
sydbox_config_get_sandbox_network (void)
{
    return config->sandbox_network;
}

gboolean
sydbox_config_get_colourise_output (void)
{
    return config->colourise_output;
}

void
sydbox_config_set_colourise_output (gboolean colourise)
{
    config->colourise_output = colourise;
}

gboolean
sydbox_config_get_allow_magic_commands (void)
{
    return config->allow_magic_commands;
}

void
sydbox_config_set_allow_magic_commands (gboolean allow)
{
    config->allow_magic_commands = allow;
}

gboolean
sydbox_config_get_paranoid_mode_enabled (void)
{
    return config->paranoid_mode_enabled;
}

void
sydbox_config_set_paranoid_mode_enabled (gboolean enabled)
{
    config->paranoid_mode_enabled = enabled;
}

const GSList *
sydbox_config_get_write_prefixes (void)
{
    return config->write_prefixes;
}

const GSList *
sydbox_config_get_predict_prefixes (void)
{
    return config->predict_prefixes;
}

