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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <confuse.h>

#include "log.h"
#include "defs.h"
#include "loop.h"
#include "path.h"
#include "util.h"
#include "trace.h"
#include "children.h"
#include "wrappers.h"
#include "syscall.h"

static context_t *ctx = NULL;
static int lock = -1;
static int net = -1;
static GSList *write_prefixes = NULL;
static GSList *predict_prefixes = NULL;


static gint verbosity = -1;

static gboolean dump;
static gboolean version;
static gboolean paranoid;

static gchar *logfile;
static gchar *config_file;

static gboolean set_lock = FALSE;

static GOptionEntry entries[] =
{
    { "config",    'c', 0, G_OPTION_ARG_FILENAME,                     &config_file, "Path to the configuration file",  NULL },
    { "dump",      'D', 0, G_OPTION_ARG_NONE,                         &dump,        "Dump configuration and exit",     NULL },
    { "lock",      'L', 0, G_OPTION_ARG_NONE,                         &set_lock,    "Disallow magic commands",         NULL },
    { "log-level", '0', 0, G_OPTION_ARG_INT,                          &verbosity,   "Logging verbosity",               NULL },
    { "log-file",  'l', 0, G_OPTION_ARG_FILENAME,                     &logfile,     "Path to the log file",            NULL },
    { "no-colour", 'C', 0, G_OPTION_ARG_NONE | G_OPTION_FLAG_REVERSE, &colour,      "Disabling colouring of messages", NULL },
    { "paranoid",  'p', 0, G_OPTION_ARG_NONE,                         &paranoid,    "Paranoid mode (EXPERIMENTAL)",    NULL },
    { "version",   'V', 0, G_OPTION_ARG_NONE,                         &version,     "Show version information",        NULL },
    { NULL, -1, 0, 0, NULL, NULL, NULL },
};

// Cleanup functions
static void cleanup(void) {
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "cleaning up before exit");
    if (NULL != ctx && NULL != ctx->eldest) {
        g_message ("killing child %i", ctx->eldest->pid);
        if (0 > trace_kill(ctx->eldest->pid) && ESRCH != errno)
            g_warning ("failed to kill child %i: %s", ctx->eldest->pid, strerror(errno));
    }
    if (NULL != ctx) {
        context_free(ctx);
        ctx = NULL;
    }
    sydbox_log_fini ();
}

static void sig_cleanup(int signum) {
    struct sigaction action;
    g_warning ("received signal %d, calling cleanup()", signum);
    cleanup();
    sigaction(signum, NULL, &action);
    action.sa_handler = SIG_DFL;
    sigaction(signum, &action, NULL);
    raise(signum);
}


static int parse_config(const char *path) {
    cfg_opt_t prefixes_opts[] = {
        CFG_INT("net", 1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t sydbox_opts[] = {
        CFG_BOOL("colour", 1, CFGF_NONE),
        CFG_STR("log_file", NULL, CFGF_NONE),
        CFG_INT("log_level", -1, CFGF_NONE),
        CFG_BOOL("paranoid", 0, CFGF_NONE),
        CFG_BOOL("lock", 0, CFGF_NONE),
        CFG_SEC("prefixes", prefixes_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_END()
    };

    cfg_t *cfg = cfg_init(sydbox_opts, CFGF_NONE);

    if (CFG_PARSE_ERROR == cfg_parse(cfg, path)) {
        g_free (cfg);
        return 0;
    }

    if (NULL == logfile && NULL != cfg_getstr(cfg, "log_file")) {
        char *lf = cfg_getstr(cfg, "log_file");
        logfile = g_strdup (lf);
    }

    if (NULL == logfile) {
        const char *env_log = g_getenv(ENV_LOG);
        if (NULL != env_log)
            logfile = g_strdup(env_log);
    }

    if (verbosity == -1) {
        verbosity = cfg_getint (cfg, "log_level");
        if (verbosity == -1)
            verbosity = 0;
    }

    if (-1 == colour) {
        if (NULL != getenv(ENV_NO_COLOUR))
            colour = 0;
        else
            colour = cfg_getbool(cfg, "colour");
    }

    if (-1 == ctx->paranoid)
        ctx->paranoid = cfg_getbool(cfg, "paranoid");

    if (-1 == lock)
        lock = cfg_getbool(cfg, "lock") ? LOCK_SET : LOCK_UNSET;

    if (-1 == net)
        net = g_getenv(ENV_NET) ? FALSE : TRUE;

    sydbox_log_init (logfile, verbosity);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "initializing path list using configuration file");
    cfg_t *cfg_prefixes;
    for (unsigned int l = 0; l < cfg_size(cfg, "prefixes"); l++) {
        cfg_prefixes = cfg_getnsec(cfg, "prefixes", l);
        for (unsigned int m = 0; m < cfg_size(cfg_prefixes, "write"); m++)
            pathnode_new(&write_prefixes, cfg_getnstr(cfg_prefixes, "write", m), 1);
        for (unsigned int n = 0; n < cfg_size(cfg_prefixes, "predict"); n++)
            pathnode_new(&predict_prefixes, cfg_getnstr(cfg_prefixes, "predict", n), 1);
        if (-1 == net)
            cfg_getint(cfg_prefixes, "net");
    }
    cfg_free (cfg);
    return 1;
}

static void dump_config(void) {
    fprintf(stderr, "config_file = %s\n", config_file);
    fprintf(stderr, "paranoid = %s\n", ctx->paranoid ? "yes" : "no");
    fprintf(stderr, "colour = %s\n", colour ? "true" : "false");
    fprintf(stderr, "log_file = %s\n", NULL == logfile ? "stderr" : logfile);
    fprintf (stderr, "log_level = %d\n", verbosity);
    fprintf(stderr, "network sandboxing = %s\n", net ? "off" : "on");
    GSList *walk;
    fprintf(stderr, "write allowed paths:\n");
    walk = write_prefixes;
    while (NULL != walk) {
        fprintf(stderr, "> %s\n", (char *) walk->data);
        walk = g_slist_next(walk);
    }
    fprintf(stderr, "write predicted paths:\n");
    walk = predict_prefixes;
    while (NULL != walk) {
        fprintf(stderr, "> %s\n", (char *) walk->data);
        walk = g_slist_next(walk);
    }
}


static gchar *
get_username (void)
{
    struct passwd *pwd;
    uid_t uid;

    errno = 0;
    uid = geteuid ();
    pwd = getpwuid (uid);

    if (errno)
        return NULL;

    return g_strdup (pwd->pw_name);
}

static gchar *
get_groupname (void)
{
    struct group *grp;
    gid_t gid;

    errno = 0;
    gid = getegid ();
    grp = getgrgid (gid);

    if (errno)
        return NULL;

    return g_strdup (grp->gr_name);
}

static void
sydbox_execute_child (int argc G_GNUC_UNUSED, char **argv)
{
    if (trace_me () < 0)
        _die (EX_SOFTWARE, "failed to set tracing: %s", g_strerror (errno));

    /* stop and wait for the parent to resume us with trace_syscall */
    if (kill (getpid (), SIGSTOP) < 0)
        _die (EX_SOFTWARE, "failed to send SIGSTOP: %s", g_strerror (errno));

    if (strncmp (argv[0], "/bin/bash", 9) == 0)
        g_fprintf (stderr, PINK PINK_FLOYD NORMAL);

    execvp (argv[0], argv);
    _die (EX_DATAERR, "execve failed: %s", g_strerror (errno));
}

static int
sydbox_execute_parent (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED, pid_t pid)
{
    int status, retval;
    struct sigaction new_action, old_action;

    new_action.sa_handler = sig_cleanup;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

#define HANDLE_SIGNAL(signal)                           \
    do {                                                \
        sigaction ((signal), NULL, &old_action);        \
        if (old_action.sa_handler != SIG_IGN)           \
            sigaction ((signal), &new_action, NULL);    \
    } while (0)

    HANDLE_SIGNAL(SIGABRT);
    HANDLE_SIGNAL(SIGSEGV);
    HANDLE_SIGNAL(SIGINT);
    HANDLE_SIGNAL(SIGTERM);

#undef HANDLE_SIGNAL

    /* wait for SIGSTOP */
    wait (&status);
    if (WIFEXITED (status))
        die (WEXITSTATUS (status), "wtf? child died before sending SIGSTOP");
    g_assert (WIFSTOPPED (status) && SIGSTOP == WSTOPSIG (status));

    if (trace_setup (pid) < 0)
        DIESOFT ("failed to setup tracing options: %s", g_strerror (errno));

    tchild_new (&(ctx->children), pid);
    ctx->eldest = childtab[pid];
    ctx->eldest->cwd = g_strdup (ctx->cwd);
    ctx->eldest->sandbox->net = net;
    ctx->eldest->sandbox->lock = lock;
    ctx->eldest->sandbox->write_prefixes = write_prefixes;
    ctx->eldest->sandbox->predict_prefixes = predict_prefixes;

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "child %lu is ready to go, resuming", (gulong) pid);
    if (trace_syscall (pid, 0) < 0) {
        trace_kill (pid);
        DIESOFT ("failed to resume eldest child %lu: %s", (gulong) pid, g_strerror (errno));
    }

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "entering loop");
    retval = trace_loop (ctx);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "exited loop with return value: %d", retval);

    syscall_free();
    return retval;
}

static int
sydbox_internal_main (int argc, char **argv)
{
    gchar *config;
    pid_t pid;


    syscall_init();
    ctx = context_new();
    ctx->paranoid = -1;

    g_atexit (cleanup);


    if (config_file)
        config = g_strdup (config_file);
    else if (g_getenv (ENV_CONFIG))
        config = g_strdup (g_getenv (ENV_CONFIG));
    else
        config = g_strdup (SYSCONFDIR G_DIR_SEPARATOR_S "sydbox.conf");

    if (! parse_config (config)) {
        g_printerr ("parse error in file '%s'", config);
        g_free (config);
        return EXIT_FAILURE;
    }

    g_free (config);


    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
           "extending path list using environment variable " ENV_WRITE);
    pathlist_init (&write_prefixes, g_getenv (ENV_WRITE));

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
           "extending path list using environment variable " ENV_PREDICT);
    pathlist_init (&predict_prefixes, g_getenv (ENV_PREDICT));

    if (dump) {
        /* sydbox_config_write_to_stderr (); */
        dump_config ();
        return EXIT_SUCCESS;
    }

    if (verbosity > 3) {
        gchar *username = NULL, *groupname = NULL;
        GString *command = NULL;

        if (! (username = get_username ())) {
            g_printerr ("failed to get password file entry: %s", g_strerror (errno));
            return EXIT_FAILURE;
        }

        if (! (groupname = get_groupname ())) {
            g_printerr ("failed to get group file entry: %s", g_strerror (errno));
            g_free (username);
            return EXIT_FAILURE;
        }

        command = g_string_new ("");
        for (gint i = 0; i < argc; i++)
            g_string_append_printf (command, "%s ", argv[i]);
        g_string_truncate (command, command->len - 1);

        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
               "forking to execute '%s' as %s:%s", command->str, username, groupname);

        g_free (username);
        g_free (groupname);
        g_string_free (command, TRUE);
    }

    if ((pid = fork()) < 0) {
        g_printerr ("failed to fork: %s", g_strerror (errno));
        return EXIT_FAILURE;
    }

    if (pid == 0)
        sydbox_execute_child (argc, argv);
    else
        return sydbox_execute_parent (argc, argv, pid);
}

static int
sandbox_main (int argc, char **argv)
{
    int retval;
    char **sandbox_argv;

    if (argc < 2) {
        sandbox_argv = g_malloc0 (2 * sizeof (char *));
        sandbox_argv[0] = g_strdup ("/bin/bash");
    } else {
        sandbox_argv = g_strdupv (&argv[1]);
    }

    retval = sydbox_internal_main (argc, sandbox_argv);

    g_strfreev (sandbox_argv);

    return retval;
}

static int
sydbox_main (int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new ("-- command [args]");
    g_option_context_add_main_entries (context, entries, PACKAGE);
    g_option_context_set_summary (context, PACKAGE "-" VERSION GIT_HEAD " - ptrace based sandbox");

    if (! g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("option parsing failed: %s\n", error->message);
        g_option_context_free (context);
        g_error_free (error);
        return EXIT_FAILURE;
    }
    g_option_context_free (context);

    if (version) {
        g_printerr (PACKAGE "-" VERSION GIT_HEAD "\n");
        return EXIT_SUCCESS;
    }

    if (! dump) {
        if (! argv[1]) {
            g_printerr ("no command given\n");
            return EXIT_FAILURE;
        }

        argc--;
        argv++;
    }

    if (paranoid)
        ctx->paranoid = 1;

    if (set_lock)
        lock = LOCK_SET;

    return sydbox_internal_main (argc, argv);
}

int
main (int argc, char **argv)
{
    if (strncmp (ebasename (argv[0]), "sandbox", 8) == 0)
        return sandbox_main (argc, argv);

    return sydbox_main (argc, argv);
}

