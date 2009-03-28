/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>

#include <confuse.h>

#include "log.h"
#include "defs.h"
#include "loop.h"
#include "path.h"
#include "util.h"
#include "trace.h"
#include "children.h"
#include "wrappers.h"

static context_t *ctx = NULL;
static int lock = -1;
static int net = -1;
static GSList *write_prefixes = NULL;
static GSList *predict_prefixes = NULL;


static gboolean dump;
static gboolean version;
static gboolean paranoid;

static gchar *profile;
static gchar *config_file;

static gboolean set_lock = FALSE;

static GOptionEntry entries[] =
{
    { "config",    'c', 0, G_OPTION_ARG_FILENAME,                     &config_file, "Path to the configuration file",  NULL },
    { "dump",      'D', 0, G_OPTION_ARG_NONE,                         &dump,        "Dump configuration and exit",     NULL },
    { "lock",      'L', 0, G_OPTION_ARG_NONE,                         &set_lock,    "Disallow magic commands",         NULL },
    { "log-file",  'l', 0, G_OPTION_ARG_FILENAME,                     &log_file,    "Path to the log file",            NULL },
    { "no-colour", 'C', 0, G_OPTION_ARG_NONE | G_OPTION_FLAG_REVERSE, &colour,      "Disabling colouring of messages", NULL },
    { "paranoid",  'p', 0, G_OPTION_ARG_NONE,                         &paranoid,    "Paranoid mode (EXPERIMENTAL)",    NULL },
    { "profile",   'P', 0, G_OPTION_ARG_STRING,                       &profile,     "Specify profile",                 NULL },
    { "version",   'V', 0, G_OPTION_ARG_NONE,                         &version,     "Show version information",        NULL },
    { NULL },
};

// Cleanup functions
static void cleanup(void) {
    LOGV("Cleaning up before exit");
    if (NULL != ctx && NULL != ctx->eldest) {
        LOGN("Killing child %i", ctx->eldest->pid);
        if (0 > trace_kill(ctx->eldest->pid) && ESRCH != errno)
            LOGW("Failed to kill child %i: %s", ctx->eldest->pid, strerror(errno));
    }
    if (NULL != log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

static void sig_cleanup(int signum) {
    struct sigaction action;
    LOGW("Received signal %d, calling cleanup()", signum);
    cleanup();
    sigaction(signum, NULL, &action);
    action.sa_handler = SIG_DFL;
    sigaction(signum, &action, NULL);
    raise(signum);
}

static bool legal_profile(const char *path) {
    if (0 == strncmp(path, "colour", 4))
        return false;
    else if (0 == strncmp(path, "log_file", 9))
        return false;
    else if (0 == strncmp(path, "log_level", 10))
        return false;
    else if (0 == strncmp(path, "paranoid", 9))
        return false;
    else if (0 == strncmp(path, "lock", 5))
        return false;
    else
        return true;
}

static int parse_config(const char *path) {
    cfg_opt_t default_opts[] = {
        CFG_INT("net", 1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t profile_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
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
        CFG_SEC("default", default_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC(profile, profile_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_END()
    };

    cfg_t *cfg = cfg_init(sydbox_opts, CFGF_NONE);

    if (CFG_PARSE_ERROR == cfg_parse(cfg, path)) {
        g_free (cfg);
        return 0;
    }

    if (NULL == log_file && NULL != cfg_getstr(cfg, "log_file")) {
        char *lf = cfg_getstr(cfg, "log_file");
        log_file = g_strdup (lf);
    }

    if (-1 == log_level) {
        log_level = cfg_getint(cfg, "log_level");
        if (-1 == log_level)
            log_level = LOG_NORMAL;
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

    LOGV("Initializing path list using configuration file");
    cfg_t *cfg_default, *cfg_profile;
    for (unsigned int i = 0; i < cfg_size(cfg, profile); i++) {
        cfg_profile = cfg_getnsec(cfg, profile, i);
        for (unsigned int j = 0; j < cfg_size(cfg_profile, "write"); j++)
            pathnode_new(&write_prefixes, cfg_getnstr(cfg_profile, "write", j), 1);
        for (unsigned int k = 0; k < cfg_size(cfg_profile, "predict"); k++)
            pathnode_new(&predict_prefixes, cfg_getnstr(cfg_profile, "predict", k), 1);
        net = cfg_getint(cfg_profile, "net");
    }
    for (unsigned int l = 0; l < cfg_size(cfg, "default"); l++) {
        cfg_default = cfg_getnsec(cfg, "default", l);
        for (unsigned int m = 0; m < cfg_size(cfg_default, "write"); m++)
            pathnode_new(&write_prefixes, cfg_getnstr(cfg_default, "write", m), 1);
        for (unsigned int n = 0; n < cfg_size(cfg_default, "predict"); n++)
            pathnode_new(&predict_prefixes, cfg_getnstr(cfg_default, "predict", n), 1);
        if (-1 == net)
            cfg_getint(cfg_default, "net");
    }
    cfg_free (cfg);
    return 1;
}

static void dump_config(void) {
    fprintf(stderr, "config_file = %s\n", config_file);
    fprintf(stderr, "paranoid = %s\n", ctx->paranoid ? "yes" : "no");
    fprintf(stderr, "profile = %s\n", profile);
    fprintf(stderr, "colour = %s\n", colour ? "true" : "false");
    fprintf(stderr, "log_file = %s\n", NULL == log_file ? "stderr" : log_file);
    fprintf(stderr, "log_level = ");
    switch (log_level) {
        case LOG_ERROR:
            fprintf(stderr, "LOG_ERROR\n");
            break;
        case LOG_WARNING:
            fprintf(stderr, "LOG_WARNING\n");
            break;
        case LOG_NORMAL:
            fprintf(stderr, "LOG_NORMAL\n");
            break;
        case LOG_VERBOSE:
            fprintf(stderr, "LOG_VERBOSE\n");
            break;
        case LOG_DEBUG:
            fprintf(stderr, "LOG_DEBUG\n");
            break;
    }
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

static const char *get_username(void) {
    uid_t uid;
    struct passwd *pwd;

    uid = geteuid();
    errno = 0;
    pwd = getpwuid(uid);

    return 0 == errno ? pwd->pw_name : NULL;
}

static const char *get_groupname(void) {
    gid_t gid;
    struct group *grp;

    errno = 0;
    gid = getegid();
    grp = getgrgid(gid);

    return 0 == errno ? grp->gr_name : NULL;
}

int
main (int argc, char **argv)
{
    char **argv_bash = NULL;

    GError *error = NULL;
    GOptionContext *context;
    gboolean parse_arguments = TRUE;
    pid_t pid;

    ctx = context_new();
    ctx->paranoid = -1;

    atexit(cleanup);

    char *bname = ebasename(argv[0]);
    if (0 == strncmp(bname, "sandbox", 8)) {
        // Aliased to sandbox
        if (2 > argc) {
            // Use /bin/bash as default program
            argv_bash = (char **) g_malloc (2 * sizeof(char *));
            argv_bash[0] = (char *) g_strndup ("/bin/bash", 10);
            argv_bash[1] = NULL;
        }
        else {
            argv++;
            argc--;
        }
        parse_arguments = TRUE;
    }

    if (parse_arguments) {
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

        if (paranoid) {
            ctx->paranoid = 1;
        }

        if (set_lock) {
            lock = LOCK_SET;
        }

        if (version) {
            g_printerr (PACKAGE "-" VERSION GIT_HEAD "\n");
            return EXIT_SUCCESS;
        }

        if (! dump) {
            if (! argv[1]) {
                g_printerr ("no command given");
                return EXIT_FAILURE;
            }

            argc--;
            argv++;
        }
    }

    if (NULL == profile) {
        profile = getenv(ENV_PROFILE);
        if (NULL == profile)
            profile = "__no_such_profile";
    }
    if (!legal_profile(profile))
        DIEUSER("invalid profile '%s' (reserved name)", profile);

    // Parse configuration file
    if (NULL == config_file)
        config_file = getenv(ENV_CONFIG);
    if (NULL == config_file)
        config_file = SYSCONFDIR"/sydbox.conf";
    if (!parse_config(config_file))
        DIEUSER("Parse error in file %s", config_file);

    // Parse environment variables
    char *log_env, *write_env, *predict_env, *net_env;
    log_env = getenv(ENV_LOG);
    write_env = getenv(ENV_WRITE);
    predict_env = getenv(ENV_PREDICT);
    net_env = getenv(ENV_NET);

    if (NULL == log_file && NULL != log_env)
        log_file = g_strdup (log_env);

    LOGV("Extending path list using environment variable "ENV_WRITE);
    pathlist_init(&write_prefixes, write_env);
    LOGV("Extending path list using environment variable "ENV_PREDICT);
    pathlist_init(&predict_prefixes, predict_env);
    if (NULL != net_env)
        net = 0;

    if (dump) {
        dump_config();
        return EXIT_SUCCESS;
    }

    int cmdsize = 1024;
    char cmd[1024] = { 0 };
    if (NULL == argv_bash) {
        for (int i = 0; i < argc; i++) {
            strncat(cmd, argv[i], cmdsize);
            if (argc - 1 != i)
                strncat(cmd, " ", 1);
            cmdsize -= (strlen(argv[i]) + 1);
        }
    }
    else
        strncpy(cmd, argv_bash[0], strlen(argv_bash[0]));

    // Get user name and group name
    const char *username = get_username();
    if (NULL == username)
        DIESOFT("Failed to get password file entry: %s", strerror(errno));
    const char *groupname = get_groupname();
    if (NULL == groupname)
        DIESOFT("Failed to get group file entry: %s", strerror(errno));

    LOGV("Forking to execute '%s' as %s:%s profile: %s", cmd, username, groupname, profile);
    pid = fork();
    if (0 > pid)
        DIESOFT("Failed to fork: %s", strerror(errno));
    else if (0 == pid) { // Child process
        if (0 > trace_me())
            _die(EX_SOFTWARE, "Failed to set tracing: %s", strerror(errno));
        // Stop and wait the parent to resume us with trace_syscall
        if (0 > kill(getpid(), SIGSTOP))
            _die(EX_SOFTWARE, "Failed to send SIGSTOP: %s", strerror(errno));
        // Start the fun!
        if (NULL != argv_bash) {
            fprintf(stderr, PINK PINK_FLOYD NORMAL);
            execvp(argv_bash[0], argv_bash);
        }
        else
            execvp(argv[0], argv);
        _die(EX_DATAERR, "execve failed: %s", strerror(errno));
    }
    else { // Parent process
        int status, ret;

        // Handle signals
        struct sigaction new_action, old_action;
        new_action.sa_handler = sig_cleanup;
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = 0;

        sigaction (SIGABRT, NULL, &old_action);
        if (SIG_IGN != old_action.sa_handler)
            sigaction(SIGABRT, &new_action, NULL);
        sigaction (SIGSEGV, NULL, &old_action);
        if (SIG_IGN != old_action.sa_handler)
            sigaction(SIGSEGV, &new_action, NULL);
        sigaction (SIGINT, NULL, &old_action);
        if (SIG_IGN != old_action.sa_handler)
            sigaction(SIGINT, &new_action, NULL);
        sigaction (SIGTERM, NULL, &old_action);
        if (SIG_IGN != old_action.sa_handler)
            sigaction(SIGTERM, &new_action, NULL);

        // Wait for the SIGSTOP
        wait(&status);
        if (WIFEXITED(status))
            die(WEXITSTATUS(status), "wtf? child died before sending SIGSTOP");
        assert(WIFSTOPPED(status) && SIGSTOP == WSTOPSIG(status));

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        if (0 > trace_setup(pid))
            DIESOFT("Failed to setup tracing options: %s", strerror(errno));
        ctx->eldest->sandbox->lock = lock;
        ctx->eldest->sandbox->net = net;
        ctx->eldest->sandbox->write_prefixes = write_prefixes;
        ctx->eldest->sandbox->predict_prefixes = predict_prefixes;
        ctx->eldest->cwd = g_strdup (ctx->cwd);

        LOGV("Child %i is ready to go, resuming", pid);
        if (0 > trace_syscall(pid, 0)) {
            trace_kill(pid);
            DIESOFT("Failed to resume eldest child %i: %s", pid, strerror(errno));
        }
        LOGV("Entering loop");
        ret = trace_loop(ctx);
        LOGV("Exit loop with return %d", ret);
        return ret;
    }
}

