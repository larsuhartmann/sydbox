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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <confuse.h>

#include "defs.h"

static char *config_file = NULL;
static char *phase = NULL;

#define MAX_PHASES 9
const char *phases[MAX_PHASES] = {
    "default", "loadenv", "saveenv", "unpack", "prepare",
    "configure", "compile", "test", "install"
};

static void about(void) {
    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fputc('\n', stderr);
}

static void usage(void) {
    int i;

    fprintf(stderr, PACKAGE"-"VERSION);
    if (0 != strlen(GITHEAD))
        fprintf(stderr, "-"GITHEAD);
    fprintf(stderr, " ptrace based sandbox\n");
    fprintf(stderr, "Usage: "PACKAGE" [options] -- command [args]\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-h, --help\t\tYou're looking at it :)\n");
    fprintf(stderr, "\t-V, --version\t\tShow version information\n");
    fprintf(stderr, "\t-p, --paranoid\t\tParanoid mode (EXPERIMENTAL)\n");
    fprintf(stderr, "\t-v, --verbose\t\tBe verbose\n");
    fprintf(stderr, "\t-d, --debug\t\tEnable debug messages\n");
    fprintf(stderr, "\t-C, --nocolour\t\tDisable colouring of messages\n");
    fprintf(stderr, "\t-P PHASE, --phase=PHASE\tSpecify phase (required)\n");
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
    fprintf(stderr, "\nPhases:\n\t");
    for (i = 0; i < MAX_PHASES - 2; i++)
        fprintf(stderr, "%s, ", phases[i]);
    fprintf(stderr, "%s\n", phases[++i]);
    fprintf(stderr, "\nParanoid Mode:\n");
    fprintf(stderr, "\tIn this mode, sydbox tries hard to ensure security of the sandbox.\n");
    fprintf(stderr, "\tFor example if a system call's path argument is a symlink, sydbox\n");
    fprintf(stderr, "\twill attempt to change it with the resolved path to prevent symlink races.\n");
    fprintf(stderr, "\tWith this mode on many packages are known to fail. An example is bash with\n");
    fprintf(stderr, "\tmalloc debugger enabled.\n");
    fprintf(stderr, "\tWithout this mode on, sydbox is NOT considered to be a security tool.\n");
    fprintf(stderr, "\tIt just helps package managers like paludis to ensure nothing wrong\n");
    fprintf(stderr, "\thappens during package installs. It's NOT meant to be a protection against\n");
    fprintf(stderr, "\tevil upstream or evil packagers.\n");
    fprintf(stderr, "\tAnd the sea isn't green, and i love the queen, and\n");
    fprintf(stderr, "\twhat exactly is a dream, what exactly is a joke?\n");
}

void cleanup(void) {
    trace_kill(ctx->eldest->pid);
    if (NULL != log_fp)
        fclose(log_fp);
}

void sig_cleanup(int signum) {
    LOGE("Received signal %d, cleaning up", signum);
    cleanup();
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
    struct tchild *child;

    tchild_new(&(ctx->children), pid);
    if (0 > trace_setup(pid)) {
        if (ESRCH == errno) { // Child died
            child = tchild_find(&(ctx->children), pid);
            return handle_esrch(ctx, child);
        }
        else
            DIESOFT("Failed to set tracing options: %s", strerror(errno));
    }
    return 0;
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
    else
        LOGC("Resumed child %i", child->pid);
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
        event = tchild_event(child, status);
//        assert((NULL == child && E_SETUP_PREMATURE == event)
//            || (NULL != child && E_SETUP_PREMATURE != event));

        switch(event) {
            case E_SETUP:
                LOGD("Latest event for child %i is E_SETUP, calling event handler", pid);
                ret = xsetup(child);
                if (0 != ret) {
                    LOGV("Event handler returned nonzero for event E_SETUP, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_SETUP for child %i", pid);
                break;
            case E_SETUP_PREMATURE:
                LOGD("Latest event for child %i is E_SETUP_PREMATURE, calling event handler", pid);
                ret = xsetup_premature(pid);
                if (0 != ret) {
                    LOGV("Event handler returned nonzero for event E_SETUP_PREMATURE, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_SETUP_PREMATURE for child %i", pid);
                break;
            case E_SYSCALL:
                LOGC("Latest event for child %i is E_SYSCALL, calling event handler", pid);
                if (NULL != child) {
                    ret = syscall_handle(ctx, child);
                    if (0 != ret) {
                        LOGD("Syscall handler returned nonzero, exiting loop");
                        return ret;
                    }
                    ret = xsyscall(child);
                    if (0 != ret) {
                        LOGD("Event handler returned nonzero for event E_SYSCALL, exiting loop");
                        return ret;
                    }
                }
                else if (0 > trace_syscall(pid, 0) && ESRCH != errno) {
                    LOGE("Failed to resume child %i before syscall: %s", pid, strerror(errno));
                    DIESOFT("Failed to resume child %i before syscall: %s", pid, strerror(errno));
                }
                LOGC("Successfully handled event E_SYSCALL for child %i", pid);
                break;
            case E_FORK:
                LOGD("Latest event for child %i is E_FORK, calling event handler", pid);
                ret = xfork(child);
                if (0 != ret) {
                    LOGD("Event handler returned non-zero for event E_EXECV, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_FORK for child %i", pid);
                break;
            case E_EXECV:
                LOGD("Latest event for child %i is E_EXECV, calling event handler", pid);
                ret = xsyscall(child);
                if (0 != ret) {
                    LOGV("Event handler returned nonzero for event E_EXECV, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_EXECV for child %i", pid);
                break;
            case E_GENUINE:
                LOGD("Latest event for child %i is E_GENUINE, calling event handler", pid);
                ret = xgenuine(child, status);
                if (0 != ret) {
                    LOGV("Event handler returned nonzero for event E_GENUINE, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_GENUINE for child %i", pid);
                break;
            case E_EXIT:
                if (ctx->eldest == child) {
                    // Eldest child, keep the return value
                    ret = WEXITSTATUS(status);
                    LOGN("Eldest child %i exited with return code %d", pid, ret);
                    tchild_delete(&(ctx->children), pid);
                    return ret;
                }
                LOGV("Child %i exited with return code: %d", pid, WEXITSTATUS(status));
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
                if (0 != ret) {
                    LOGV("Event handler returned nonzero for event E_UNKNOWN, exiting loop");
                    return ret;
                }
                LOGD("Successfully handled event E_UNKNOWN for child %i", pid);
                break;
        }
    }
    return ret;
}

static int legal_phase(const char *phase) {
    for (int i = 0; i < MAX_PHASES; i++) {
        if (0 == strncmp(phase, phases[i], strlen(phases[i]) + 1))
            return 1;
    }
    return 0;
}

static int parse_config(const char *path) {
    cfg_opt_t default_opts[] = {
        CFG_INT("net", 1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t loadenv_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t saveenv_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t unpack_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t prepare_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t configure_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t compile_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t test_opts[] = {
        CFG_INT("net", -1, CFGF_NONE),
        CFG_STR_LIST("write", "{}", CFGF_NONE),
        CFG_STR_LIST("predict", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_opt_t install_opts[] = {
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

    if (CFG_PARSE_ERROR == cfg_parse(cfg, path)) {
        free(cfg);
        return 0;
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

    if (0 == ctx->paranoid)
        ctx->paranoid = cfg_getbool(cfg, "paranoid");

    LOGV("Initializing path list using configuration file");
    cfg_t *cfg_default, *cfg_phase;
    for (int i = 0; i < cfg_size(cfg, phase); i++) {
        cfg_phase = cfg_getnsec(cfg, phase, i);
        for (int i = 0; i < cfg_size(cfg_phase, "write"); i++)
            pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_phase, "write", i));
        for (int i = 0; i < cfg_size(cfg_phase, "predict"); i ++)
            pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_phase, "predict", i));
        ctx->net_allowed = cfg_getint(cfg_phase, "net");
    }
    if (0 != strncmp(phase, "default", 8)) {
        for (int i = 0; i < cfg_size(cfg, "default"); i++) {
            cfg_default = cfg_getnsec(cfg, "default", i);
            for (int i = 0; i < cfg_size(cfg_default, "write"); i++)
                pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_default, "write", i));
            for (int i = 0; i < cfg_size(cfg_default, "predict"); i++)
                pathnode_new(&(ctx->write_prefixes), cfg_getnstr(cfg_default, "predict", i));
            if (-1 == ctx->net_allowed)
                cfg_getint(cfg_default, "net");
        }
    }
    cfg_free(cfg);
    return 1;
}

static void dump_config(void) {
    fprintf(stderr, "config_file = %s\n", config_file);
    fprintf(stderr, "paranoid = %s\n", ctx->paranoid ? "yes" : "no");
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
        fprintf(stderr, "> %s\n", curnode->path);
        curnode = curnode->next;
    }
    fprintf(stderr, "write predicted paths:\n");
    curnode = ctx->predict_prefixes;
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
    colour = -1;
    dump = 0;
    atexit(cleanup);

    char *basec = xstrndup(argv[0], PATH_MAX);
    char *bname = basename(basec);
    if (0 == strncmp(bname, "sandbox", 8)) {
        // Aliased to sandbox
        free(basec);
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
    else
        free(basec);

    // Parse command line
    static struct option long_options[] = {
        {"version",  no_argument, NULL, 'V'},
        {"help",     no_argument, NULL, 'h'},
        {"paranoid", no_argument, NULL, 'p'},
        {"verbose",  no_argument, NULL, 'v'},
        {"debug",    no_argument, NULL, 'd'},
        {"nocolour", no_argument, NULL, 'C'},
        {"phase",    required_argument, NULL, 'P'},
        {"log-file", required_argument, NULL, 'l'},
        {"config",   required_argument, NULL, 'c'},
        {"dump",     no_argument, NULL, 'D'},
        {0, 0, NULL, 0}
    };

    while (-1 != (optc = getopt_long(argc, argv, "hVpvdCP:l:c:D", long_options, NULL))) {
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
    if (NULL == phase) {
        phase = getenv(ENV_PHASE);
        if (NULL == phase)
            phase = "default";
    }
    if (!legal_phase(phase))
        DIEUSER("invalid phase '%s'", phase);

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

    if ('\0' == log_file[0] && NULL != log_env)
        strncpy(log_file, log_env, PATH_MAX);

    LOGV("Extending path list using environment variable "ENV_WRITE);
    pathlist_init(&(ctx->write_prefixes), write_env);
    LOGV("Extending path list using environment variable "ENV_PREDICT);
    pathlist_init(&(ctx->predict_prefixes), predict_env);
    if (NULL != net_env)
        ctx->net_allowed = 0;

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

    LOGV("Forking to execute '%s' as %s:%s", cmd, username, groupname);
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
        _die(EX_DATAERR, strerror(errno));
    }
    else { // Parent process
        int status, ret;

        signal(SIGINT, sig_cleanup);
        signal(SIGTERM, sig_cleanup);

        // Wait for the SIGSTOP
        wait(&status);
        if (WIFEXITED(status))
            die(WEXITSTATUS(status), "wtf? child died before sending SIGSTOP");
        assert(WIFSTOPPED(status) && SIGSTOP == WSTOPSIG(status));

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        if (0 > trace_setup(pid))
            DIESOFT("Failed to setup tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
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
