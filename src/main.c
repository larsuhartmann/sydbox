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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defs.h"

void about(void) {
    fprintf(stderr, "%s-%s\n", PACKAGE, VERSION);
}

void usage(void) {
    fprintf(stderr, "%s-%s ptrace based sandbox\n", PACKAGE, VERSION);
    fprintf(stderr, "Usage: %s [options] -- command [args]\n\n", PACKAGE);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-h, --help\tYou're looking at it :)\n");
    fprintf(stderr, "\t-V, --version\tShow version information\n");
    fprintf(stderr, "\t-v, --verbose\tBe verbose\n");
    fprintf(stderr, "\t-d, --debug\tEnable debug messages\n");
    fprintf(stderr, "\nEnvironment variables:\n");
    fprintf(stderr, "\t"ENV_WRITE": Colon separated path prefixes of write allowed paths\n");
    fprintf(stderr, "\t"ENV_PREDICT": Colon separated path prefixes of write predicted paths\n");
    fprintf(stderr, "\t"ENV_NET": If set network calls will NOT be allowed\n");
    fprintf(stderr, "\t"ENV_NO_COLOUR": If set messages won't be coloured\n");
}

int trace_loop(context_t *ctx) {
    int status, ret;
    unsigned int event;
    pid_t pid, childpid;
    struct tchild *child;

    ret = EXIT_SUCCESS;
    while (NULL != ctx->children) {
        pid = waitpid(-1, &status, __WALL);
        if (0 > pid) {
            lg(LOG_ERROR, "main.trace_loop.waitpid", "waitpid failed for child %i", pid);
            return EX_SOFTWARE;
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
                    return EX_SOFTWARE;
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
                            "Failed to resume child %i after syscall: %s", child->pid, strerror(errno));
                    return EX_SOFTWARE;
                }
                break;
            case E_FORK:
                /* Get new child's pid */
                if (0 != ptrace(PTRACE_GETEVENTMSG, pid, NULL, &childpid)) {
                    lg(LOG_ERROR, "main.trace_loop.geteventmsg",
                            "Failed to get the pid of the newborn child: %s", strerror(errno));
                    return EX_SOFTWARE;
                }
                if (tchild_find(&(ctx->children), childpid)) {
                    /* Child is prematurely born, let it continue its life */
                    if (0 != ptrace(PTRACE_SYSCALL, childpid, NULL, NULL)) {
                        lg(LOG_ERROR, "main.trace_loop.resume_premature",
                                "Failed to resume prematurely born child %i: %s",
                                pid, strerror(errno));
                        return EX_SOFTWARE;
                    }
                }
                else {
                    /* Add the child, setup will be done later */
                    tchild_new(&(ctx->children), childpid);
                }
                /* No break so this calls PTRACE_SYSCALL */
            case E_EXECV:
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_execve",
                            "Failed to resume child %i after execve: %s", pid, strerror(errno));
                    return EX_SOFTWARE;
                }
                break;
            case E_GENUINE:
                if (0 != ptrace(PTRACE_SYSCALL, pid, 0, WSTOPSIG(status))) {
                    lg(LOG_ERROR, "main.trace_loop.resume_genuine",
                            "Failed to resume child %i after genuine signal: %s",
                            pid, strerror(errno));
                    return EX_SOFTWARE;
                }
                break;
            case E_EXIT:
                if (ctx->eldest == child) {
                    /* Eldest child, keep the return value */
                    ret = WEXITSTATUS(status);
                    lg(LOG_VERBOSE, "main.trace_loop.eldest_dead",
                            "Eldest child %i exited with return code %d", pid, ret);
                }
                tchild_delete(&(ctx->children), pid);
                break;
            case E_EXIT_SIGNAL:
                if (0 != ptrace(PTRACE_SYSCALL, pid, 0, WTERMSIG(status))) {
                    lg(LOG_ERROR, "main.trace_loop.resume_exit_signal",
                            "Failed to resume child %i after signal: %s",
                            pid, strerror(errno));
                    return EX_SOFTWARE;
                }
                ret = 1;
                tchild_delete(&(ctx->children), pid);
                break;
            case E_UNKNOWN:
                lg(LOG_WARNING, "main.trace_loop.unknown", 
                        "Unknown signal %#x received from child %i", status, pid);
                if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
                    lg(LOG_ERROR, "main.trace_loop.resume_unknown",
                            "Failed to resume child %i after unknown signal %#x: %s",
                            pid, status, strerror(errno));
                    return EX_SOFTWARE;
                }
                break;
        }
    }
    return ret;
}

int main(int argc, char **argv) {
    int optc;
    static struct option long_options[] = {
        {"version", no_argument, NULL, 'V'},
        {"help",    no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"debug",   no_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };
    pid_t pid;

    log_level = LOG_NORMAL;
    while (-1 != (optc = getopt_long(argc, argv, "hVvd", long_options, NULL))) {
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
            case '?':
            default:
                die(EX_USAGE, "try %s --help for more information", PACKAGE);
        }
    }

    if (argc < optind + 1)
        die(EX_USAGE, "no command given");
    else if (0 != strncmp("--", argv[optind - 1], 3))
        die(EX_USAGE, "expected '--' instead of '%s'", argv[optind]);
    else
        argv += optind;

    lg(LOG_VERBOSE, "main.fork", "Forking");
    pid = fork();
    if (0 > pid)
        die(EX_SOFTWARE, strerror(errno));
    else if (0 == pid) { /* Child process */
        if (0 != ptrace(PTRACE_TRACEME, 0, NULL, NULL))
            die(EX_SOFTWARE, "couldn't set tracing: %s", strerror(errno));
        /* Stop and wait the parent to resume us with PTRACE_SYSCALL */
        if (0 > kill(getpid(), SIGTRAP))
            die(EX_SOFTWARE, "failed to send SIGTRAP: %s", strerror(errno));
        /* Start the fun! */
        execvp(argv[0], argv);
        die(EX_DATAERR, strerror(errno));
        return EXIT_FAILURE;
    }
    else { /* Parent process */
        int status, ret;
        char *write_env, *predict_env;
        context_t *ctx;

        lg(LOG_VERBOSE, "main.context_init", "Initializing context");
        ctx = context_new();
        write_env = getenv(ENV_WRITE);
        predict_env = getenv(ENV_PREDICT);
        ctx->net_allowed = getenv(ENV_NET) ? 0 : 1;
        pathlist_init(&(ctx->write_prefixes), write_env);
        pathlist_init(&(ctx->predict_prefixes), predict_env);
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        tchild_setup(ctx->eldest);


        lg(LOG_VERBOSE, "main.context_init.done", "Initialization done, waiting for child");
        /* Wait for the SIGTRAP */
        wait(&status);
        if (WIFEXITED(status)) {
            /* wtf? child died before sending SIGTRAP */
            ret = WEXITSTATUS(status);
            goto exit;
        }
        lg(LOG_VERBOSE, "main.resume", "Child %i is ready to go, resuming", pid);
        if (0 != ptrace(PTRACE_SYSCALL, pid, NULL, NULL)) {
            ptrace(PTRACE_KILL, pid, NULL, NULL);
            lg(LOG_ERROR, "main.resume.fail", "Failed to resume child %i: %s", pid, strerror(errno));
            ret = EX_SOFTWARE;
            goto exit;
        }
        ret = trace_loop(ctx);
exit:
        if (NULL != ctx)
            context_free(ctx);
        return ret;
    }
}
