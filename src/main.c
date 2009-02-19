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
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <confuse.h>

#include "defs.h"

context_t *ctx = NULL;

void about(void) {
    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fputc('\n', stderr);
}

void usage(void) {
    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fprintf(stderr, " ptrace based sandbox\n");
    fprintf(stderr, "Usage: "PACKAGE" [options] -- command [args]\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-h, --help\t\tYou're looking at it :)\n");
    fprintf(stderr, "\t-V, --version\t\tShow version information\n");
    fprintf(stderr, "\t-v, --verbose\t\tBe verbose\n");
    fprintf(stderr, "\t-d, --debug\t\tEnable debug messages\n");
    fprintf(stderr, "\t-C, --nocolour\t\tDisable colouring of messages\n");
    fprintf(stderr, "\t-p PHASE, --phase=PHASE\tSpecify phase (required)\n");
    fprintf(stderr, "\t-c PATH, --config=PATH\tSpecify PATH to the configuration file\n");
    fprintf(stderr, "\t-l PATH, --log-file=PATH\n\t\t\t\tSpecify PATH to the log file\n");
    fprintf(stderr, "\t-D, --dump\t\tDump configuration and exit\n");
    fprintf(stderr, "\nEnvironment variables:\n");
    fprintf(stderr, "\t"ENV_PHASE":\t\tSpecify phase, can be used instead of -p\n");
    fprintf(stderr, "\t"ENV_WRITE":\t\tColon seperated paths to allow write\n");
    fprintf(stderr, "\t"ENV_PREDICT":\tColon seperated paths to predict write\n");
    fprintf(stderr, "\t"ENV_NET":\t\tEnable sandboxing of network connections\n");
    fprintf(stderr, "\t"ENV_CONFIG":\t\tSpecify PATH to the configuration file\n");
    fprintf(stderr, "\t"ENV_NO_COLOUR":\tIf set messages won't be coloured\n");
    fprintf(stderr, "\t"ENV_LOG":\t\tSpecify PATH to the log file\n");
    fprintf(stderr, "\nPhases:\n");
    fprintf(stderr, "\tdefault, loadenv, saveenv, unpack, prepare, configure, compile, test, install\n");
}

int trace_loop(void) {
    int status, ret;
    unsigned int event;
    pid_t pid, childpid;
    struct tchild *child;

    ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL);
        if (0 > pid) {
            lg(LOG_ERROR, "main.trace_loop.waitpid", "waitpid failed for child %i: %s",
                    pid, strerror(errno));
            die(EX_SOFTWARE, "waitpid failed for child %i: %s",
                    pid, strerror(errno));
        }
        child = tchild_find(&(ctx->children), pid);
        event = tchild_event(child, status);
        assert((NULL == child && E_SETUP_PREMATURE == event)
                || (NULL != child && E_SETUP_PREMATURE != event));

        switch(event) {
            case E_SETUP:
                tchild_setup(child);
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_after_setup",
                            "Failed to resume child %i after setup: %s",
                            child->pid, strerror(errno));
                    die(EX_SOFTWARE, "Failed to resume child %i after setup: %s",
                            child->pid, strerror(errno));
                }
                break;
            case E_SETUP_PREMATURE:
                tchild_new(&(ctx->children), pid);
                tchild_setup(ctx->children);
                break;
            case E_SYSCALL:
                if (NULL != child)
                    syscall_handle(ctx, child);

                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_after_syscall",
                            "Failed to resume child %i after syscall: %s",
                            child->pid, strerror(errno));
                    die(EX_SOFTWARE, "Failed to resume child %i after syscall: %s",
                            child->pid, strerror(errno));
                }
                break;
            case E_FORK:
                /* Get new child's pid */
                if (0 != ptrace(PTRACE_GETEVENTMSG, pid, NULL, &childpid)) {
                    lg(LOG_ERROR, "main.trace_loop.geteventmsg",
                            "Failed to get the pid of the newborn child: %s",
                            strerror(errno));
                    die(EX_SOFTWARE, "Failed to get the pid of the newborn child: %s",
                            strerror(errno));
                }
                if (tchild_find(&(ctx->children), childpid)) {
                    /* Child is prematurely born, let it continue its life */
                    if (0 != ptrace(PTRACE_SYSCALL, childpid, NULL, NULL)) {
                        lg(LOG_ERROR, "main.trace_loop.resume_premature",
                                "Failed to resume prematurely born child %i: %s",
                                pid, strerror(errno));
                        die(EX_SOFTWARE, "Failed to resume prematurely born child %i: %s",
                                pid, strerror(errno));
                    }
                }
                else {
                    /* Add the child, setup will be done later */
                    tchild_new(&(ctx->children), childpid);
                }
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_fork",
                            "Failed to resume child %i after fork, vfork or clone: %s",
                            pid, strerror(errno));
                    die(EX_SOFTWARE, "Failed to resume child %i after fork: %s",
                            pid, strerror(errno));
                }
                break;
            case E_EXECV:
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_execve",
                            "Failed to resume child %i after execve: %s",
                            pid, strerror(errno));
                    die(EX_SOFTWARE, "Failed to resume child %i after execve: %s",
                            pid, strerror(errno));
                }
                break;
            case E_GENUINE:
                if (0 != ptrace(PTRACE_SYSCALL, pid, 0, WSTOPSIG(status))) {
                    lg(LOG_ERROR, "main.trace_loop.resume_genuine",
                            "Failed to resume child %i after genuine signal: %s",
                            pid, strerror(errno));
                    die(EX_SOFTWARE, "Failed to resume child %i after genuine signal: %s",
                            pid, strerror(errno));
                }
                break;
            case E_EXIT:
                if (ctx->eldest == child) {
                    /* Eldest child, keep the return value */
                    ret = WEXITSTATUS(status);
                    lg(LOG_VERBOSE, "main.trace_loop.eldest_dead",
                            "Eldest child %i exited with return code %d", pid, ret);
                    tchild_delete(&(ctx->children), pid);
                    return ret;
                }
                tchild_delete(&(ctx->children), pid);
                break;
            case E_EXIT_SIGNAL:
                if (ctx->eldest == child) {
                    tchild_delete(&(ctx->children), pid);
                    return EXIT_FAILURE;
                }
                tchild_delete(&(ctx->children), pid);
                break;
            case E_UNKNOWN:
                lg(LOG_DEBUG, "main.trace_loop.unknown",
                        "Unknown signal %#x received from child %i", status, pid);
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_unknown",
                            "Failed to resume child %i after unknown signal %#x: %s",
                            pid, status, strerror(errno));
                    die(EX_SOFTWARE,
                            "Failed to resume child %i after unknown signal %#x: %s",
                            pid, status, strerror(errno));
                }
                break;
        }
    }
    return ret;
}

void cleanup(void) {
    if (NULL != ctx)
        context_free(ctx);
    if (NULL != flog)
        fclose(flog);
}

int main(int argc, char **argv) {
    int optc, dump;
    char *config_file = NULL;
    char *phase = NULL;
    ctx = context_new();

    /* Parse command line */
    static struct option long_options[] = {
        {"version",  no_argument, NULL, 'V'},
        {"help",     no_argument, NULL, 'h'},
        {"verbose",  no_argument, NULL, 'v'},
        {"debug",    no_argument, NULL, 'd'},
        {"nocolour", no_argument, NULL, 'C'},
        {"phase",    required_argument, NULL, 'p'},
        {"log-file", required_argument, NULL, 'l'},
        {"config",   required_argument, NULL, 'c'},
        {"dump",     no_argument, NULL, 'D'},
        {0, 0, NULL, 0}
    };
    pid_t pid;

    colour = -1;
    log_level = -1;
    dump = 0;
    log_file[0] = '\0';
    flog = NULL;
    atexit(cleanup);
    while (-1 != (optc = getopt_long(argc, argv, "hVvdCp:l:c:D", long_options, NULL))) {
        switch (optc) {
            case 'h':
                usage();
                return EXIT_SUCCESS;
            case 'V':
                about();
                return EXIT_SUCCESS;
            case 'v':
                log_level = LOG_VERBOSE;
                break;
            case 'd':
                log_level = LOG_DEBUG;
                break;
            case 'C':
                colour = 0;
                break;
            case 'p':
                phase = optarg;
                break;
            case 'l':
                strncpy(log_file, optarg, PATH_MAX);
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'D':
                dump = 1;
                break;
            case '?':
            default:
                die(EX_USAGE, "try %s --help for more information", PACKAGE);
        }
    }

    if (!dump) {
        if (argc < optind + 1)
            die(EX_USAGE, "no command given");
        else if (0 != strncmp("--", argv[optind - 1], 3))
            die(EX_USAGE, "expected '--' instead of '%s'", argv[optind]);
        else {
            argc -= optind;
            argv += optind;
        }
    }

    if (NULL == phase) {
        phase = getenv(ENV_PHASE);
        if (NULL == phase)
            phase = "default";
    }
    if (0 != strncmp(phase, "default", 8) &&
        0 != strncmp(phase, "loadenv", 8) &&
        0 != strncmp(phase, "saveenv", 8) &&
        0 != strncmp(phase, "unpack", 7) &&
        0 != strncmp(phase, "prepare", 8) &&
        0 != strncmp(phase, "configure", 10) &&
        0 != strncmp(phase, "compile", 8) &&
        0 != strncmp(phase, "test", 5) &&
        0 != strncmp(phase, "install", 8))
        die(EX_USAGE, "invalid phase '%s'", phase);

    /* Parse configuration file */
    if (NULL == config_file)
        config_file = getenv(ENV_CONFIG);
    if (NULL == config_file)
        config_file = SYSCONFDIR"/sydbox.conf";

    cfg_opt_t default_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", 1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t loadenv_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t saveenv_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t unpack_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t prepare_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t configure_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t compile_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t test_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t install_opts[] = {
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_INT("net", -1, CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t sydbox_opts[] = {
        CFG_BOOL("colour", 1, CFGF_NONE),
        CFG_STR("log_file", NULL, CFGF_NONE),
        CFG_INT("log_level", -1, CFGF_NONE),
        CFG_SEC("default", default_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("loadenv", loadenv_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("saveenv", saveenv_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("unpack", unpack_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("unpack", unpack_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("prepare", prepare_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("configure", configure_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("compile", compile_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("test", test_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_SEC("install", install_opts, CFGF_TITLE | CFGF_MULTI),
        CFG_END()
    };

    cfg_t *cfg = cfg_init(sydbox_opts, CFGF_NONE);

    if (CFG_PARSE_ERROR == cfg_parse(cfg, config_file)) {
        free(cfg);
        die(EX_USAGE, "Parse error in file %s", config_file);
    }

    if ('\0' == log_file[0] && NULL != cfg_getstr(cfg, "log_file"))
        strncpy(log_file, cfg_getstr(cfg, "log_file"), PATH_MAX);

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

    cfg_t *cfg_default, *cfg_phase;
    for (int i = 0; i < cfg_size(cfg, phase); i++) {
        cfg_phase = cfg_getnsec(cfg, phase, i);
        for (int i = 0; i < cfg_size(cfg_phase, "write"); i++)
            pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_phase, "write", i));
        for (int i = 0; i < cfg_size(cfg_phase, "predict"); i ++)
            pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_phase, "write", i));
        ctx->net_allowed = cfg_getint(cfg_phase, "net");
    }
    if (0 != strncmp(phase, "default", 8)) {
        for (int i = 0; i < cfg_size(cfg, "default"); i++) {
            cfg_default = cfg_getnsec(cfg, "default", i);
            for (int i = 0; i < cfg_size(cfg_default, "write"); i++)
                pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_default, "write", i));
            for (int i = 0; i < cfg_size(cfg_default, "predict"); i++)
                pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_default, "write", i));
            if (-1 == ctx->net_allowed)
                cfg_getint(cfg_default, "net");
        }
    }
    cfg_free(cfg);

    /* Parse environment variables */
    char *log_env, *write_env, *predict_env, *net_env;
    log_env = getenv(ENV_LOG);
    write_env = getenv(ENV_WRITE);
    predict_env = getenv(ENV_PREDICT);
    net_env = getenv(ENV_NET);

    if ('\0' == log_file[0] && NULL != log_env)
        strncpy(log_file, log_env, PATH_MAX);

    lg(LOG_DEBUG, "main.env_write", "Initializing path list using "ENV_WRITE);
    pathlist_init(&(ctx->write_prefixes), write_env);
    lg(LOG_DEBUG, "main.env_predict", "Initializing path list using "ENV_PREDICT);
    pathlist_init(&(ctx->predict_prefixes), predict_env);
    if (NULL != net_env)
        ctx->net_allowed = 0;

    if (dump) {
        fprintf(stderr, "config_file = %s\n", config_file);
        fprintf(stderr, "phase = %s\n", phase);
        fprintf(stderr, "colour = %s\n", colour ? "true" : "false");
        fprintf(stderr, "log_file = %s\n", '\0' == log_file[0] ? "stderr" : log_file);
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
        fprintf(stderr, "network sandboxing = %s\n", ctx->net_allowed ? "off" : "on");
        struct pathnode *curnode;
        fprintf(stderr, "write allowed paths:\n");
        curnode = ctx->write_prefixes;
        while (NULL != curnode) {
            fprintf(stderr, "> %s\n", curnode->pathname);
            curnode = curnode->next;
        }
        fprintf(stderr, "write predicted paths:\n");
        curnode = ctx->predict_prefixes;
        while (NULL != curnode) {
            fprintf(stderr, "> %s\n", curnode->pathname);
            curnode = curnode->next;
        }
        return EXIT_SUCCESS;
    }

    int cmdsize = 4096;
    char cmd[4096] = { 0 };
    for (int i = 0; i < argc; i++) {
        strncat(cmd, argv[i], cmdsize);
        if (argc - 1 != i)
            strncat(cmd, " ", 1);
        cmdsize -= (strlen(argv[i]) + 1);
    }

    /* Get user name and group name */
    uid_t uid;
    gid_t gid;
    struct passwd *pwd;
    struct group *grp;

    uid = geteuid();
    errno = 0;
    pwd = getpwuid(uid);
    if (NULL == pwd)
        die(EX_SOFTWARE, "Failed to get password file entry: %s", strerror(errno));
    errno = 0;
    gid = getegid();
    grp = getgrgid(gid);
    if (NULL == getgrgid)
        die(EX_SOFTWARE, "Failed to get group file entry: %s", strerror(errno));

    lg(LOG_VERBOSE, "main.fork", "Forking to execute '%s' as %s:%s",
            cmd, pwd->pw_name, grp->gr_name);
    pid = fork();
    if (0 > pid)
        die(EX_SOFTWARE, strerror(errno));
    else if (0 == pid) { /* Child process */
        if (0 > ptrace(PTRACE_TRACEME, 0, NULL, NULL))
            _die(EX_SOFTWARE, "couldn't set tracing: %s", strerror(errno));
        /* Stop and wait the parent to resume us with PTRACE_SYSCALL */
        if (0 > kill(getpid(), SIGSTOP))
            _die(EX_SOFTWARE, "failed to send SIGSTOP: %s", strerror(errno));
        /* Start the fun! */
        execvp(argv[0], argv);
        _die(EX_DATAERR, strerror(errno));
    }
    else { /* Parent process */
        int status, ret;

        /* Wait for the SIGSTOP */
        wait(&status);
        if (WIFEXITED(status))
            die(WEXITSTATUS(status), "wtf? child died before sending SIGSTOP");
        assert(WIFSTOPPED(status) && SIGSTOP == WSTOPSIG(status));

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        tchild_setup(ctx->eldest);

        lg(LOG_VERBOSE, "main.resume", "Child %i is ready to go, resuming", pid);
        if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
            ptrace(PTRACE_KILL, pid, NULL, NULL);
            die(EX_SOFTWARE, "Failed to resume eldest child %i: %s", pid, strerror(errno));
        }
        return trace_loop();
    }
}
