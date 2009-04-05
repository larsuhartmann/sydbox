/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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

#include <confuse.h>

#include "defs.h"
#include "path.h"
#include "sydbox-config.h"

struct sydbox_config
{
    gchar *logfile;

    gint verbosity;

    gboolean sandbox_network;
    gboolean colourize_output;
    gboolean allow_magic_commands;
    gboolean paranoid_mode_enabled;

    GSList *write_prefixes;
    GSList *predict_prefixes;
} *config;


static cfg_opt_t prefixes_opts[] =
{
    CFG_BOOL ("net", FALSE, CFGF_NONE),
    CFG_STR_LIST ("write", "{}", CFGF_NONE),
    CFG_STR_LIST ("predict", "{}", CFGF_NONE),
    CFG_END (),
};

static cfg_opt_t sydbox_opts[] =
{
    CFG_BOOL ("colour", TRUE, CFGF_NONE),
    CFG_STR ("log_file", NULL, CFGF_NONE),
    CFG_INT ("log_level", 0, CFGF_NONE),
    CFG_BOOL ("paranoid", FALSE, CFGF_NONE),
    CFG_BOOL ("lock", FALSE, CFGF_NONE),
    CFG_SEC ("prefixes", prefixes_opts, CFGF_TITLE | CFGF_MULTI),
    CFG_END (),
};


gboolean
sydbox_config_load (const gchar * const file)
{
    cfg_t *sydbox_config, *profile_config;
    gchar *config_file;

    g_return_val_if_fail (! config, TRUE);

    if (file)
        config_file = g_strdup (file);
    else if (g_getenv (ENV_CONFIG))
        config_file = g_strdup (g_getenv (ENV_CONFIG));
    else
        config_file = g_strdup (SYSCONFDIR G_DIR_SEPARATOR_S "sydbox.conf");

    sydbox_config = cfg_init (sydbox_opts, CFGF_NONE);

    if (cfg_parse (sydbox_config, config_file) == CFG_PARSE_ERROR) {
        g_printerr ("failed to parse config file: '%s'", config_file);
        cfg_free (sydbox_config);
        g_free (config_file);
        return FALSE;
    }

    config = g_new0 (struct sydbox_config, 1);

    if (g_getenv (ENV_LOG))
        config->logfile = g_strdup (g_getenv (ENV_LOG));
    else if (cfg_getstr (sydbox_config, "log_file"))
        config->logfile = g_strdup (cfg_getstr (sydbox_config, "log_file"));

    config->verbosity = cfg_getint (sydbox_config, "log_level");

    if (g_getenv (ENV_NO_COLOUR))
        config->colourize_output = FALSE;
    else
        config->colourize_output = cfg_getbool (sydbox_config, "colour");

    config->paranoid_mode_enabled = cfg_getbool (sydbox_config, "paranoid");

    config->allow_magic_commands = cfg_getbool (sydbox_config, "lock");

    for (guint i = 0; i < cfg_size (sydbox_config, "prefixes"); i++) {
        guint j;

        profile_config = cfg_getnsec (sydbox_config, "prefixes", i);

        for (j = 0; j < cfg_size (profile_config, "write"); j++)
            pathnode_new (&config->write_prefixes, cfg_getnstr (profile_config, "write", j), 1);

        for (j = 0; j < cfg_size (profile_config, "predict"); j++)
            pathnode_new (&config->predict_prefixes, cfg_getnstr (profile_config, "predict", j), 1);

        config->sandbox_network = cfg_getbool (profile_config, "net");
    }

    cfg_free (sydbox_config);
    g_free (config_file);

    return TRUE;
}

void
sydbox_config_update_from_environment (void)
{
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
           "extending path list using environment variable " ENV_WRITE);

    pathlist_init (&config->write_prefixes, g_getenv (ENV_WRITE));

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
           "extending path list using environment variable " ENV_PREDICT);

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
    g_fprintf (stderr, "colour = %s\n", config->colourize_output ? "yes" : "no");
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

