/* Sydbox testcases for syscall.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include <check.h>

#include <../src/defs.h>
#include "check_sydbox.h"

START_TEST(check_syscall_check_chmod_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_DENY_VIOLATION == decs.res,
                "Expected R_DENY_VIOLATION, got %d", decs.res);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(check_syscall_check_chmod_predict) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        pathlist_init(&(ctx->predict_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_DENY_RETURN == decs.res,
                "Expected R_DENY_RETURN, got %d", decs.res);
        fail_unless(0 == decs.ret,
                "Expected 0 got %d", decs.ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chmod_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_ALLOW== decs.res,
                "Expected R_ALLOW, got %d", decs.res);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chmod_predict_write_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        pathlist_init(&(ctx->predict_prefixes), "/home/see:/usr/emily:/tmp/play");
        pathlist_init(&(ctx->write_prefixes), "/home/arnold:/dev/layne");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_DENY_VIOLATION == decs.res,
                "Expected R_DENY_VIOLATION, got %d", decs.res);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(check_syscall_check_chmod_predict_write_predict) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        pathlist_init(&(ctx->predict_prefixes), "/home/emily:/dev:/tmp");
        pathlist_init(&(ctx->write_prefixes), "/home/arnold:/dev/layne");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_DENY_RETURN == decs.res,
                "Expected R_DENY_RETURN, got %d", decs.res);
        fail_unless(0 == decs.ret,
                "Expected 0 got %d", decs.ret);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chmod_predict_write_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
    }
    else { /* parent */
        int status, syscall;
        context_t *ctx = context_new();
        struct decision decs;

        pathlist_init(&(ctx->predict_prefixes), "/home/see:/usr/emily:/tmp/play");
        pathlist_init(&(ctx->write_prefixes), "/var/tmp:/dev");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        tchild_setup(ctx->eldest);

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        syscall = ptrace_get_syscall(pid);
        decs = syscall_check(ctx, ctx->eldest, syscall);
        fail_unless(R_ALLOW == decs.res,
                "Expected R_ALLOW, got %d", decs.res);

        kill(pid, SIGTERM);
    }
}
END_TEST

Suite *syscall_suite_create(void) {
    Suite *s = suite_create("syscall");

    /* syscall_check test cases */
    TCase *tc_syscall_check = tcase_create("syscall_check");
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_predict_write_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_predict_write_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_predict_write_allow);
    suite_add_tcase(s, tc_syscall_check);

    return s;
}
