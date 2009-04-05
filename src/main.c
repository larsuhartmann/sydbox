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

#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sydbox-config.h"

#include "log.h"
#include "defs.h"
#include "loop.h"
#include "path.h"
#include "util.h"
#include "trace.h"
#include "children.h"

static context_t *ctx = NULL;
static int lock = -1;


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

static void G_GNUC_NORETURN
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
    if (WIFEXITED (status)) {
        g_printerr ("wtf? child died before sending SIGSTOP");
        exit (WEXITSTATUS (status));
    }
    g_assert (WIFSTOPPED (status) && SIGSTOP == WSTOPSIG (status));

    if (trace_setup (pid) < 0) {
        g_printerr ("failed to setup tracing options: %s", g_strerror (errno));
        exit (-1);
    }

    tchild_new (&(ctx->children), pid);
    ctx->eldest = childtab[pid];
    ctx->eldest->cwd = g_strdup (ctx->cwd);
    ctx->eldest->sandbox->net = sydbox_config_get_sandbox_network ();
    ctx->eldest->sandbox->lock = lock;
    ctx->eldest->sandbox->write_prefixes = sydbox_config_get_write_prefixes ();
    ctx->eldest->sandbox->predict_prefixes = sydbox_config_get_predict_prefixes ();

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "child %lu is ready to go, resuming", (gulong) pid);
    if (trace_syscall (pid, 0) < 0) {
        trace_kill (pid);
        g_printerr ("failed to resume eldest child %lu: %s", (gulong) pid, g_strerror (errno));
        exit (-1);
    }

    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "entering loop");
    retval = trace_loop (ctx);
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "exited loop with return value: %d", retval);

    return retval;
}

static int
sydbox_internal_main (int argc, char **argv)
{
    pid_t pid;


    ctx = context_new();
    ctx->paranoid = -1;


    g_atexit (cleanup);

    if (! sydbox_config_load (config_file))
        return EXIT_FAILURE;

    /* if the verbosity is specified, update our configuration */
    if (verbosity != -1)
        sydbox_config_set_verbosity (verbosity);

    sydbox_log_init (sydbox_config_get_log_file (), sydbox_config_get_verbosity ());

    sydbox_config_update_from_environment ();

    if (dump) {
        sydbox_config_write_to_stderr ();
        return EXIT_SUCCESS;
    }

    if (sydbox_config_get_verbosity () > 3) {
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
    if (strncmp (basename (argv[0]), "sandbox", 8) == 0)
        return sandbox_main (argc, argv);

    return sydbox_main (argc, argv);
}

