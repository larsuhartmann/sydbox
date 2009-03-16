/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 * Based in part upon catbox which is:
 *  Copyright (c) 2006-2007 TUBITAK/UEKAE
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
#include <limits.h>
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

#include <confuse.h>

#include "defs.h"

static context_t *ctx = NULL;
static char *config_file = NULL;
static char *profile = NULL;
static int lock = -1;
static int net = -1;
static struct pathnode *write_prefixes = NULL;
static struct pathnode *predict_prefixes = NULL;

static void about(void) {
    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fputc('\n', stderr);
}

static void usage(void) {
    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fprintf(stderr, " ptrace based sandbox\n");
    fprintf(stderr, "Usage: "PACKAGE" [options] -- command [args]\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-h, --help\t\tYou're looking at it :)\n");
    fprintf(stderr, "\t-V, --version\t\tShow version information\n");
    fprintf(stderr, "\t-p, --paranoid\t\tParanoid mode (EXPERIMENTAL)\n");
    fprintf(stderr, "\t-L, --lock\t\tDisallow magic commands\n");
    fprintf(stderr, "\t-v, --verbose\t\tBe verbose\n");
    fprintf(stderr, "\t-d, --debug\t\tEnable debug messages\n");
    fprintf(stderr, "\t-C, --nocolour\t\tDisable colouring of messages\n");
    fprintf(stderr, "\t-P PROFILE,\n\t  --profile=PROFILE\tSpecify profile\n");
    fprintf(stderr, "\t-c PATH, --config=PATH\tSpecify PATH to the configuration file\n");
    fprintf(stderr, "\t-l PATH, --log-file=PATH\n\t\t\t\tSpecify PATH to the log file\n");
    fprintf(stderr, "\t-D, --dump\t\tDump configuration and exit\n");
    fprintf(stderr, "\nEnvironment variables:\n");
    fprintf(stderr, "\t"ENV_PROFILE":\tSpecify profile, can be used instead of -P\n");
    fprintf(stderr, "\t"ENV_WRITE":\t\tColon seperated paths to allow write\n");
    fprintf(stderr, "\t"ENV_PREDICT":\tColon seperated paths to predict write\n");
    fprintf(stderr, "\t"ENV_NET":\t\tEnable sandboxing of network connections\n");
    fprintf(stderr, "\t"ENV_CONFIG":\t\tSpecify PATH to the configuration file\n");
    fprintf(stderr, "\t"ENV_NO_COLOUR":\tIf set messages won't be coloured\n");
    fprintf(stderr, "\t"ENV_LOG":\t\tSpecify PATH to the log file\n");
    fprintf(stderr, "\nParanoid Mode:\n");
    fprintf(stderr, "\tIn this mode, sydbox tries hard to ensure security of the sandbox.\n");
    fprintf(stderr, "\tFor example if a system call's path argument is a symlink, sydbox\n");
    fprintf(stderr, "\twill attempt to change it with the resolved path to prevent symlink races.\n");
    fprintf(stderr, "\tWith this mode on many packages are known to fail.\n");
    fprintf(stderr, "\tWithout this mode on, sydbox is NOT considered to be a security tool.\n");
    fprintf(stderr, "\tIt just helps package managers like paludis to ensure nothing wrong\n");
    fprintf(stderr, "\thappens during package installs. It's NOT meant to be a protection against\n");
    fprintf(stderr, "\tevil upstream or evil packagers.\n");
    fprintf(stderr, "\tAnd the sea isn't green, and i love the queen, and\n");
    fprintf(stderr, "\twhat exactly is a dream, what exactly is a joke?\n");
}

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

// Event handlers
static int xsetup(struct tchild *child) {
    if (0 > trace_setup(child->pid)) {
        if (ESRCH == errno) // Child died
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to set tracing options: %s", strerror(errno));
    }
    else
        child->flags &= ~TCHILD_NEEDSETUP;

    if (0 > trace_syscall(child->pid, 0)) {
        if (ESRCH == errno) // Child died
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i after setup: %s", child->pid, strerror(errno));
            DIESOFT("Failed to resume child %i after setup: %s", child->pid, strerror(errno));
        }
    }
    else
        LOGC("Resumed child %i after setup", child->pid);
    return 0;
}

static int xsetup_premature(pid_t pid) {
    tchild_new(&(ctx->children), pid);
    return xsetup(ctx->children);
}

static int xsyscall(struct tchild *child) {
    if (0 > trace_syscall(child->pid, 0)) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i: %s", child->pid, strerror(errno));
            DIESOFT("Failed to resume child %i: %s", child->pid, strerror(errno));
        }
    }
    return 0;
}

static int xfork(struct tchild *child) {
    pid_t childpid;
    struct tchild *newchild;

    // Get new child's pid
    if (0 > trace_geteventmsg(child->pid, &childpid)) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to get the pid of the newborn child: %s", strerror(errno));
    }
    else
        LOGD("The newborn child's pid is %i", childpid);

    newchild = tchild_find(&(ctx->children), childpid);
    if (NULL != newchild) {
        LOGD("Child %i is prematurely born, letting it continue its life", newchild->pid);
        if (0 > trace_syscall(newchild->pid, 0)) {
            if (ESRCH == errno)
                return handle_esrch(ctx, newchild);
            else
                DIESOFT("Failed to resume prematurely born child %i: %s", newchild->pid, strerror(errno));
        }
        else
            LOGC("Resumed prematurely born child %i", newchild->pid);
    }
    else {
        // Add the child, setup will be done later
        tchild_new(&(ctx->children), childpid);
    }
    return xsyscall(child);
}

static int xgenuine(struct tchild *child, int status) {
    if (0 > trace_syscall(child->pid, WSTOPSIG(status))) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else
            DIESOFT("Failed to resume child %i after genuine signal: %s", child->pid, strerror(errno));
    }
    else
        LOGC("Resumed child %i after genuine signal", child->pid);
    return 0;
}

static int xunknown(struct tchild *child, int status) {
    if (0 > trace_syscall(child->pid, WSTOPSIG(status))) {
        if (ESRCH == errno)
            return handle_esrch(ctx, child);
        else {
            LOGE("Failed to resume child %i after unknown signal %#x: %s", child->pid, status,
                    strerror(errno));
            DIESOFT("Failed to resume child %i after unknown signal %#x: %s", child->pid, status,
                    strerror(errno));
        }
    }
    else
        LOGC("Resumed child %i after unknown signal %#x", child->pid, status);
    return 0;
}

static int trace_loop(void) {
    int status, ret;
    unsigned int event;
    pid_t pid;
    struct tchild *child;

    ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL);
        if (0 > pid) {
            LOGE("waitpid failed: %s", strerror(errno));
            DIESOFT("waitpid failed: %s", strerror(errno));
        }
        child = tchild_find(&(ctx->children), pid);
        event = trace_event(status);
        assert(NULL != child || E_STOP == event || E_EXIT == event || E_EXIT_SIGNAL == event);

        switch(event) {
            case E_STOP:
                LOGD("Latest event for child %i is E_STOP, calling event handler", pid);
                if (NULL == child) {
                    ret = xsetup_premature(pid);
                    if (0 != ret)
                        return ret;
                }
                else {
                    ret = xsetup(child);
                    if (0 != ret)
                        return ret;
                }
                break;
            case E_SYSCALL:
                ret = syscall_handle(ctx, child);
                if (0 != ret)
                    return ret;
                ret = xsyscall(child);
                if (0 != ret)
                    return ret;
                break;
            case E_FORK:
            case E_VFORK:
            case E_CLONE:
                LOGD("Latest event for child %i is E_FORK, calling event handler", pid);
                ret = xfork(child);
                if (0 != ret)
                    return ret;
                break;
            case E_EXEC:
                LOGD("Latest event for child %i is E_EXEC, calling event handler", pid);
                ret = xsyscall(child);
                if (0 != ret)
                    return ret;
                break;
            case E_GENUINE:
                LOGD("Latest event for child %i is E_GENUINE, calling event handler", pid);
                ret = xgenuine(child, status);
                if (0 != ret)
                    return ret;
                break;
            case E_EXIT:
                ret = WEXITSTATUS(status);
                if (ctx->eldest == child) {
                    // Eldest child, keep the return value
                    if (0 != ret)
                        LOGN("Eldest child %i exited with return code %d", pid, ret);
                    else
                        LOGV("Eldest child %i exited with return code %d", pid, ret);
                    tchild_delete(&(ctx->children), pid);
                    return ret;
                }
                LOGD("Child %i exited with return code: %d", pid, ret);
                tchild_delete(&(ctx->children), pid);
                break;
            case E_EXIT_SIGNAL:
                if (ctx->eldest == child) {
                    LOGN("Eldest child %i exited with signal %d", pid, WTERMSIG(status));
                    tchild_delete(&(ctx->children), pid);
                    return EXIT_FAILURE;
                }
                LOGV("Child %i exited with signal %d", pid, WTERMSIG(status));
                tchild_delete(&(ctx->children), pid);
                break;
            case E_UNKNOWN:
                LOGV("Unknown signal %#x received from child %i", status, pid);
                ret = xunknown(child, status);
                if (0 != ret)
                    return ret;
                break;
        }
    }
    return ret;
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
        free(cfg);
        return 0;
    }

    if (NULL == log_file && NULL != cfg_getstr(cfg, "log_file")) {
        char *lf = cfg_getstr(cfg, "log_file");
        log_file = xstrdup(lf);
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
    cfg_free(cfg);
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
    struct pathnode *curnode;
    fprintf(stderr, "write allowed paths:\n");
    curnode = write_prefixes;
    while (NULL != curnode) {
        fprintf(stderr, "> %s\n", curnode->path);
        curnode = curnode->next;
    }
    fprintf(stderr, "write predicted paths:\n");
    curnode = predict_prefixes;
    while (NULL != curnode) {
        fprintf(stderr, "> %s\n", curnode->path);
        curnode = curnode->next;
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

int main(int argc, char **argv) {
    int optc, dump;
    pid_t pid;
    char **argv_bash = NULL;
    ctx = context_new();
    ctx->paranoid = -1;
    colour = -1;
    dump = 0;
    atexit(cleanup);

    char *bname = ebasename(argv[0]);
    if (0 == strncmp(bname, "sandbox", 8)) {
        // Aliased to sandbox
        if (2 > argc) {
            // Use /bin/bash as default program
            argv_bash = (char **) xmalloc(2 * sizeof(char *));
            argv_bash[0] = (char *) xstrndup("/bin/bash", 10);
            argv_bash[1] = NULL;
        }
        else {
            argv++;
            argc--;
        }
        goto skip_commandline;
    }

    // Parse command line
    static struct option long_options[] = {
        {"version",  no_argument, NULL, 'V'},
        {"help",     no_argument, NULL, 'h'},
        {"paranoid", no_argument, NULL, 'p'},
        {"lock",     no_argument, NULL, 'L'},
        {"verbose",  no_argument, NULL, 'v'},
        {"debug",    no_argument, NULL, 'd'},
        {"nocolour", no_argument, NULL, 'C'},
        {"profile",  required_argument, NULL, 'P'},
        {"log-file", required_argument, NULL, 'l'},
        {"config",   required_argument, NULL, 'c'},
        {"dump",     no_argument, NULL, 'D'},
        {0, 0, NULL, 0}
    };

    while (-1 != (optc = getopt_long(argc, argv, "hVpLvdCP:l:c:D", long_options, NULL))) {
        switch (optc) {
            case 'h':
                usage();
                return EXIT_SUCCESS;
            case 'V':
                about();
                return EXIT_SUCCESS;
            case 'p':
                ctx->paranoid = 1;
                break;
            case 'L':
                lock = LOCK_SET;
                break;
            case 'v':
                log_level = LOG_VERBOSE;
                break;
            case 'd':
                if (LOG_DEBUG == log_level)
                    log_level = LOG_DEBUG_CRAZY;
                else if (LOG_DEBUG_CRAZY != log_level)
                    log_level = LOG_DEBUG;
                break;
            case 'C':
                colour = 0;
                break;
            case 'P':
                profile = optarg;
                break;
            case 'l':
                log_file = xstrdup(optarg);
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'D':
                dump = 1;
                break;
            case '?':
            default:
                DIEUSER("try %s --help for more information", PACKAGE);
        }
    }

    if (!dump) {
        if (argc < optind + 1)
            DIEUSER("no command given");
        else if (0 != strncmp("--", argv[optind - 1], 3))
            DIEUSER("expected '--' instead of '%s'", argv[optind]);
        else {
            argc -= optind;
            argv += optind;
        }
    }

skip_commandline:
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
        log_file = xstrdup(log_env);

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

        // Clean environment
        unsetenv("PWD");

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
        ctx->eldest->cwd = egetcwd();
        if (NULL == ctx->eldest->cwd)
            DIESOFT("Failed to get current working directory: %s", strerror(errno));

        LOGV("Child %i is ready to go, resuming", pid);
        if (0 > trace_syscall(pid, 0)) {
            trace_kill(pid);
            DIESOFT("Failed to resume eldest child %i: %s", pid, strerror(errno));
        }
        LOGV("Entering loop");
        ret = trace_loop();
        LOGV("Exit loop with return %d", ret);
        return ret;
    }
}
