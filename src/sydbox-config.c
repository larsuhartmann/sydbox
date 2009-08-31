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
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "net.h"
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

    int network_mode;
    bool network_restrict_connect;

    bool colourise_output;
    bool disallow_magic_commands;
    bool wait_all;
    bool allow_proc_pid;

    GSList *filters;
    GSList *write_prefixes;
    GSList *exec_prefixes;
    GSList *network_whitelist;
} *config;


static void sydbox_config_set_defaults(void)
{
    g_assert(config != NULL);

    config->colourise_output = true;
    config->verbosity = 1;
    config->sandbox_path = true;
    config->sandbox_exec = false;
    config->sandbox_network = false;
    config->network_restrict_connect = false;
    config->network_mode = SYDBOX_NETWORK_ALLOW;
    config->disallow_magic_commands = false;
    config->wait_all = false;
    config->allow_proc_pid = true;
}

bool sydbox_config_load(const gchar * const file)
{
    const gchar *config_file;
    GKeyFile *config_fd;
    GError *config_error = NULL;

    g_return_val_if_fail(!config, true);

    // Initialize config structure
    config = g_new0(struct sydbox_config, 1);

    if (g_getenv(ENV_NO_CONFIG)) {
        /* ENV_NO_CONFIG set, set the defaults,
         * and return without parsing the configuration file.
         */
        sydbox_config_set_defaults();
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
        switch (config_error->code) {
            case G_FILE_ERROR_NOENT:
                /* Configuration file not found!
                 * Set the defaults and return true.
                 */
                g_error_free(config_error);
                g_key_file_free(config_fd);
                sydbox_config_set_defaults();
                return true;
            default:
                g_printerr("failed to parse config file: %s\n", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
        }
    }

    // Get main.colour
    config->colourise_output = g_key_file_get_boolean(config_fd, "main", "colour", &config_error);
    if (!config->colourise_output) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.colour not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get main.lock
    config->disallow_magic_commands = g_key_file_get_boolean(config_fd, "main", "lock", &config_error);
    if (!config->disallow_magic_commands && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.lock not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get main.wait_all
    config->wait_all = g_key_file_get_boolean(config_fd, "main", "wait_all", &config_error);
    if (!config->wait_all && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.wait_all not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get main.filters
    char **filterlist = g_key_file_get_string_list(config_fd, "main", "filters", NULL, NULL);
    if (NULL != filterlist) {
        for (unsigned int i = 0; NULL != filterlist[i]; i++)
            sydbox_config_addfilter(filterlist[i]);
        g_strfreev(filterlist);
    }


    // Get log.file
    config->logfile = g_key_file_get_string(config_fd, "log", "file", NULL);

    // Get log.level
    config->verbosity = g_key_file_get_integer(config_fd, "log", "level", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("log.level not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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
    config->sandbox_path = g_key_file_get_boolean(config_fd, "sandbox", "path", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("sandbox.path not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get sandbox.exec
    config->sandbox_exec = g_key_file_get_boolean(config_fd, "sandbox", "exec", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("sandbox.exec not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get sandbox.network
    config->sandbox_network = g_key_file_get_boolean(config_fd, "sandbox", "network", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("main.network not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
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

    // Get prefix.write
    char **write_prefixes = g_key_file_get_string_list(config_fd, "prefix", "write", NULL, NULL);
    if (NULL != write_prefixes) {
        for (unsigned int i = 0; NULL != write_prefixes[i]; i++)
            pathnode_new_early(&config->write_prefixes, write_prefixes[i], 1);
        g_strfreev(write_prefixes);
    }

    // Get prefix.exec
    char **exec_prefixes = g_key_file_get_string_list(config_fd, "prefix", "exec", NULL, NULL);
    if (NULL != exec_prefixes) {
        for (unsigned int i = 0; NULL != exec_prefixes[i]; i++)
            pathnode_new_early(&config->exec_prefixes, exec_prefixes[i], 1);
        g_strfreev(exec_prefixes);
    }

    // Get net.default
    gchar *netdefault = g_key_file_get_string(config_fd, "net", "default", NULL);
    if (NULL != netdefault) {
        if (0 == strncmp(netdefault, "allow", 6))
            config->network_mode = SYDBOX_NETWORK_ALLOW;
        else if (0 == strncmp(netdefault, "deny", 5))
            config->network_mode = SYDBOX_NETWORK_DENY;
        else if (0 == strncmp(netdefault, "local", 6))
            config->network_mode = SYDBOX_NETWORK_LOCAL;
        else {
            g_printerr("error: invalid value for net.default `%s'\n", netdefault);
            g_free(netdefault);
            g_key_file_free(config_fd);
            g_free(config);
            return false;
        }
        g_free(netdefault);
    }

    // Get net.restrict_connect
    config->network_restrict_connect = g_key_file_get_boolean(config_fd, "net", "restrict_connect", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                g_printerr("net.restrict_connect not a boolean: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                config->network_restrict_connect = false;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    // Get net.whitelist
    char **netwhitelist = g_key_file_get_string_list(config_fd, "net", "whitelist", NULL, NULL);
    if (NULL != netwhitelist) {
        for (unsigned int i = 0; NULL != netwhitelist[i]; i++) {
            if (0 > netlist_new_from_string(&config->network_whitelist, netwhitelist[i], false)) {
                g_printerr("error: malformed address `%s' at position %d of net.whitelist\n", netwhitelist[i], i);
                g_strfreev(netwhitelist);
                g_key_file_free(config_fd);
                g_free(config);
                return false;
            }
        }
        g_strfreev(netwhitelist);
    }

    // Cleanup and return
    g_key_file_free(config_fd);
    return true;
}

void sydbox_config_update_from_environment(void)
{
    g_info("extending path list using environment variable " ENV_WRITE);
    pathlist_init(&config->write_prefixes, g_getenv(ENV_WRITE));

    g_info("extending path list using environment variable " ENV_EXEC_ALLOW);
    pathlist_init(&config->exec_prefixes, g_getenv(ENV_EXEC_ALLOW));

    g_info("extending network whitelist using environment variable " ENV_NET_WHITELIST);
    if (g_getenv(ENV_NET_WHITELIST)) {
        char **netwhitelist = g_strsplit(g_getenv(ENV_NET_WHITELIST), ";", 0);
        for (unsigned int i = 0; NULL != netwhitelist[i]; i++) {
            if (0 > netlist_new_from_string(&config->network_whitelist, netwhitelist[i], true)) {
                g_critical("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST"\n",
                        netwhitelist[i], i);
                g_printerr("error: malformed address `%s' at position %d of "ENV_NET_WHITELIST"\n",
                        netwhitelist[i], i);
                g_strfreev(netwhitelist);
                exit(-1);
            }
        }
        g_strfreev(netwhitelist);
    }
}


static inline void print_slist_entry(gpointer data, gpointer userdata G_GNUC_UNUSED)
{
    gchar *cdata = (gchar *) data;
    if (NULL != cdata && '\0' != cdata[0])
        g_fprintf(stderr, "\t%s\n", cdata);
}

static inline void print_netlist_entry(gpointer data, gpointer userdata G_GNUC_UNUSED)
{
    struct sydbox_addr *saddr = (struct sydbox_addr *) data;

    if (NULL == saddr)
        return;

    switch (saddr->family) {
        case AF_UNIX:
            g_fprintf(stderr, "\t{family=AF_UNIX path=%s}\n", saddr->addr);
            break;
        case AF_INET:
            g_fprintf(stderr, "\t{family=AF_INET addr=%s port=%d}\n", saddr->addr, saddr->port);
            break;
        case AF_INET6:
            g_fprintf(stderr, "\t{family=AF_INET6 addr=%s port=%d}\n", saddr->addr, saddr->port);
            break;
        default:
            g_assert_not_reached();
    }
}

void sydbox_config_write_to_stderr (void)
{
    g_fprintf(stderr, "main.colour = %s\n", config->colourise_output ? "on" : "off");
    g_fprintf(stderr, "main.lock = %s\n", config->disallow_magic_commands ? "set" : "unset");
    g_fprintf(stderr, "main.wait_all = %s\n", config->wait_all ? "yes" : "no");
    g_fprintf(stderr, "main.allow_proc_pid = %s\n", config->allow_proc_pid ? "yes" : "no");
    g_fprintf(stderr, "log.file = %s\n", config->logfile ? config->logfile : "stderr");
    g_fprintf(stderr, "log.level = %d\n", config->verbosity);
    g_fprintf(stderr, "sandbox.path = %s\n", config->sandbox_path ? "yes" : "no");
    g_fprintf(stderr, "sandbox.exec = %s\n", config->sandbox_exec ? "yes" : "no");
    g_fprintf(stderr, "sandbox.network = %s\n", config->sandbox_network ? "yes" : "no");
    if (config->network_mode == SYDBOX_NETWORK_ALLOW)
        g_fprintf(stderr, "net.default = allow\n");
    else if (config->network_mode == SYDBOX_NETWORK_DENY)
        g_fprintf(stderr, "net.default = deny\n");
    else if (config->network_mode == SYDBOX_NETWORK_LOCAL)
        g_fprintf(stderr, "net.default = local\n");
    else
        g_assert_not_reached();
    g_fprintf(stderr, "net.restrict_connect = %s\n", config->network_restrict_connect ? "yes" : "no");
    g_fprintf(stderr, "prefix.write:\n");
    g_slist_foreach(config->write_prefixes, print_slist_entry, NULL);
    g_fprintf(stderr, "prefix.exec\n");
    g_slist_foreach(config->exec_prefixes, print_slist_entry, NULL);
    g_fprintf(stderr, "net.whitelist:\n");
    g_slist_foreach(config->network_whitelist, print_netlist_entry, NULL);
}


const gchar *sydbox_config_get_log_file(void)
{
    return config->logfile;
}

void sydbox_config_set_log_file(const gchar * const logfile)
{
    if (config->logfile)
        g_free(config->logfile);

    config->logfile = g_strdup(logfile);
}

gint sydbox_config_get_verbosity(void)
{
    return config->verbosity;
}

void sydbox_config_set_verbosity(gint verbosity)
{
    config->verbosity = verbosity;
}

bool sydbox_config_get_sandbox_path(void)
{
    return config->sandbox_path;
}

void sydbox_config_set_sandbox_path(bool on)
{
    config->sandbox_path = on;
}

bool sydbox_config_get_sandbox_exec(void)
{
    return config->sandbox_exec;
}

void sydbox_config_set_sandbox_exec(bool on)
{
    config->sandbox_exec = on;
}

bool sydbox_config_get_sandbox_network(void)
{
    return config->sandbox_network;
}

void sydbox_config_set_sandbox_network(bool on)
{
    config->sandbox_network = on;
}

bool sydbox_config_get_network_restrict_connect(void)
{
    return config->network_restrict_connect;
}

void sydbox_config_set_network_restrict_connect(bool on)
{
    config->network_restrict_connect = on;
}

int sydbox_config_get_network_mode(void)
{
    return config->network_mode;
}

void sydbox_config_set_network_mode(int state)
{
    config->network_mode = state;
}

bool sydbox_config_get_colourise_output(void)
{
    return config->colourise_output;
}

void sydbox_config_set_colourise_output(bool colourise)
{
    config->colourise_output = colourise;
}

bool sydbox_config_get_disallow_magic_commands(void)
{
    return config->disallow_magic_commands;
}

void sydbox_config_set_disallow_magic_commands(bool disallow)
{
    config->disallow_magic_commands = disallow;
}

bool sydbox_config_get_wait_all(void)
{
    return config->wait_all;
}

void sydbox_config_set_wait_all(bool waitall)
{
    config->wait_all = waitall;
}

bool sydbox_config_get_allow_proc_pid(void)
{
    return config->allow_proc_pid;
}

void sydbox_config_set_allow_proc_pid(bool allow)
{
    config->allow_proc_pid = allow;
}

GSList *sydbox_config_get_write_prefixes(void)
{
    return config->write_prefixes;
}

GSList *sydbox_config_get_exec_prefixes(void)
{
    return config->exec_prefixes;
}

GSList *sydbox_config_get_filters(void)
{
    return config->filters;
}

GSList *sydbox_config_get_network_whitelist(void)
{
    return config->network_whitelist;
}

void sydbox_config_set_network_whitelist(GSList *whitelist)
{
    config->network_whitelist = whitelist;
}

void sydbox_config_addfilter(const gchar *filter)
{
    config->filters = g_slist_append(config->filters, g_strdup(filter));
}

int sydbox_config_rmfilter(const gchar *filter)
{
    GSList *walk;

    walk = config->filters;
    while (NULL != walk) {
        if (0 == strncmp(walk->data, filter, strlen(filter) + 1)) {
            config->filters = g_slist_remove_link(config->filters, walk);
            g_free(walk->data);
            g_slist_free(walk);
            return 1;
        }
        walk = g_slist_next(walk);
    }
    return 0;
}

void sydbox_config_rmfilter_all(void)
{
    g_slist_foreach(config->filters, (GFunc) g_free, NULL);
    g_slist_free(config->filters);
    config->filters = NULL;
}

