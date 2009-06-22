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

#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "sydbox-log.h"
#include "sydbox-utils.h"
#include "sydbox-config.h"

#include "loop.h"
#include "path.h"
#include "trace.h"
#include "config.h"
#include "children.h"
#include "syscall.h"

/* pink floyd */
#define PINK_FLOYD  "       ..uu.                               \n" \
                    "       ?$\"\"`?i           z'              \n" \
                    "       `M  .@\"          x\"               \n" \
                    "       'Z :#\"  .   .    f 8M              \n" \
                    "       '&H?`  :$f U8   <  MP   x#'         \n" \
                    "       d#`    XM  $5.  $  M' xM\"          \n" \
                    "     .!\">     @  'f`$L:M  R.@!`           \n" \
                    "    +`  >     R  X  \"NXF  R\"*L           \n" \
                    "        k    'f  M   \"$$ :E  5.           \n" \
                    "        %%    `~  \"    `  'K  'M          \n" \
                    "            .uH          'E   `h           \n" \
                    "         .x*`             X     `          \n" \
                    "      .uf`                *                \n" \
                    "    .@8     .                              \n" \
                    "   'E9F  uf\"          ,     ,             \n" \
                    "     9h+\"   $M    eH. 8b. .8    .....     \n" \
                    "    .8`     $'   M 'E  `R;'   d?\"\"\"`\"# \n" \
                    "   ` E      @    b  d   9R    ?*     @     \n" \
                    "     >      K.zM `%%M'   9'    Xf   .f     \n" \
                    "    ;       R'          9     M  .=`       \n" \
                    "    t                   M     Mx~          \n" \
                    "    @                  lR    z\"           \n" \
                    "    @                  `   ;\"             \n" \
                    "                          `                \n"


static context_t *ctx = NULL;


static gint verbosity = -1;

static gchar *logfile;
static gchar *config_file;

static gboolean dump;
static gboolean lock;
static gboolean colour;
static gboolean version;
static gboolean paranoid;

static GOptionEntry entries[] =
{
    { "config",    'c', 0, G_OPTION_ARG_FILENAME,                     &config_file, "Path to the configuration file",  NULL },
    { "dump",      'D', 0, G_OPTION_ARG_NONE,                         &dump,        "Dump configuration and exit",     NULL },
    { "lock",      'L', 0, G_OPTION_ARG_NONE,                         &lock,        "Disallow magic commands",         NULL },
    { "log-level", '0', 0, G_OPTION_ARG_INT,                          &verbosity,   "Logging verbosity",               NULL },
    { "log-file",  'l', 0, G_OPTION_ARG_FILENAME,                     &logfile,     "Path to the log file",            NULL },
    { "no-colour", 'C', 0, G_OPTION_ARG_NONE | G_OPTION_FLAG_REVERSE, &colour,      "Disabling colouring of messages", NULL },
    { "paranoid",  'p', 0, G_OPTION_ARG_NONE,                         &paranoid,    "Paranoid mode (EXPERIMENTAL)",    NULL },
    { "version",   'V', 0, G_OPTION_ARG_NONE,                         &version,     "Show version information",        NULL },
    { NULL, -1, 0, 0, NULL, NULL, NULL },
};

// Cleanup functions
static void cleanup(void) {
    g_info ("cleaning up before exit");
    if (NULL != ctx) {
        g_info ("killing child %i", ctx->eldest);
        if (0 > trace_kill(ctx->eldest) && ESRCH != errno)
            g_warning ("failed to kill child %i: %s", ctx->eldest, strerror(errno));
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
    if (trace_me () < 0) {
        g_printerr ("failed to set tracing: %s", g_strerror (errno));
        _exit (-1);
    }

    /* stop and wait for the parent to resume us with trace_syscall */
    if (kill (getpid (), SIGSTOP) < 0) {
        g_printerr ("failed to send SIGSTOP: %s", g_strerror (errno));
        _exit (-1);
    }

    if (strncmp (argv[0], "/bin/bash", 9) == 0)
        g_fprintf (stderr, ANSI_DARK_MAGENTA PINK_FLOYD ANSI_NORMAL);

    execvp (argv[0], argv);

    g_printerr ("execvp() failed: %s\n", g_strerror (errno));
    _exit(-1);
}

static int
sydbox_execute_parent (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED, pid_t pid)
{
    int status, retval;
    struct sigaction new_action, old_action;
    struct tchild *eldest;

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

    tchild_new (&(ctx->children), pid, -1);
    ctx->eldest = pid;
    eldest = tchild_find(ctx->children, pid);
    eldest->cwd = g_strdup (ctx->cwd);
    eldest->sandbox->net = sydbox_config_get_sandbox_network ();
    eldest->sandbox->lock = sydbox_config_get_allow_magic_commands () ? LOCK_UNSET : LOCK_SET;
    eldest->sandbox->write_prefixes = sydbox_config_get_write_prefixes ();
    eldest->sandbox->predict_prefixes = sydbox_config_get_predict_prefixes ();

    g_info ("child %lu is ready to go, resuming", (gulong) pid);
    if (trace_syscall (pid, 0) < 0) {
        trace_kill (pid);
        g_printerr ("failed to resume eldest child %lu: %s", (gulong) pid, g_strerror (errno));
        exit (-1);
    }

    g_info ("entering loop");
    retval = trace_loop (ctx);
    g_info ("exited loop with return value: %d", retval);

    syscall_free();
    return retval;
}

static int
sydbox_internal_main (int argc, char **argv)
{
    pid_t pid;


    syscall_init();
    ctx = context_new ();

    g_atexit (cleanup);

    /*
     * options are loaded from config file, updated from the environment, and
     * then overridden by the user passed parameters.
     */
    if (! sydbox_config_load (config_file))
        return EXIT_FAILURE;

    if (verbosity >= 0)
        sydbox_config_set_verbosity (verbosity);

    if (logfile)
        sydbox_config_set_log_file (logfile);

    /* initialize logging as early as possible */
    sydbox_log_init ();

    sydbox_config_update_from_environment ();

    if (colour)
        sydbox_config_set_colourise_output (TRUE);

    if (lock)
        sydbox_config_set_allow_magic_commands (FALSE);

    if (paranoid)
        sydbox_config_set_paranoid_mode_enabled (TRUE);

    if (dump) {
        sydbox_config_write_to_stderr ();
        return EXIT_SUCCESS;
    }

    if (sydbox_config_get_verbosity () > 1) {
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

        g_info ("forking to execute '%s' as %s:%s", command->str, username, groupname);

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
        argc--;
        argv++;

        if (argv[0] && strcmp (argv[0], "--") == 0) {
            argc--;
            argv++;
        }

        if (! argv[0]) {
            g_printerr ("no command given\n");
            return EXIT_FAILURE;
        }
    }

    return sydbox_internal_main (argc, argv);
}

int
main (int argc, char **argv)
{
    if (strncmp (basename (argv[0]), "sandbox", 8) == 0)
        return sandbox_main (argc, argv);

    return sydbox_main (argc, argv);
}

