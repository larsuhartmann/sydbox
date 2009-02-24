/* Sydbox testcases for syscall.c
 * vim: set et ts=4 sts=4 sw=4 fdm=syntax :
 * Copyright 2009 Ali Polatel <polatel@gmail.com>
 * Distributed under the terms of the GNU General Public License v2
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include <check.h>

#include "../src/defs.h"
#include "check_sydbox.h"

void syscall_setup(void) {
    if (0 > mkdir("emily", 0755))
        fail("mkdir(\"emily\", 0755) failed: %s", strerror(errno));
}
void syscall_teardown(void) {
    unlink("emily/syd.txt");
    rmdir("emily");
}

START_TEST(check_syscall_check_chmod_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval,
                "Failed to set retval to EPERM (got %d)", ctx->eldest->retval);
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
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->predict_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected deny");
        fail_unless(0 == ctx->eldest->retval,
                "Expected 0 got %d", ctx->eldest->retval);

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
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chown_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval,
                "Failed to set retval to EPERM (got %d)", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chown_predict) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->predict_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected deny");
        fail_unless(0 == ctx->eldest->retval,
                "Expected 0 got %d", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_chown_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_rdonly_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_wronly_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval,
                "Failed to set retval to EPERM (got %d)", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(check_syscall_check_open_wronly_predict) {
    pid_t pid;
    int pfd[2];
    char cwd[PATH_MAX];
    char *rcwd;

    /* setup */
    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    if (0 > pipe(pfd))
        fail("pipe() failed: %s", strerror(errno));
    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", open("emily/syd.txt", O_WRONLY | O_CREAT));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        close(pfd[1]);

        pathnode_new(&(ctx->predict_prefixes), rcwd);
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_unless(0 == ptrace(PTRACE_CONT, pid, NULL, NULL),
                "PTRACE_CONT failed: %s", strerror(errno));

        int fd, n;
        char buf[16], proc[PATH_MAX], rpath[PATH_MAX];

        if (0 > read(pfd[0], buf, 16))
            fail("read() failed: %s", strerror(errno));
        fd = atoi(buf);

        snprintf(proc, PATH_MAX, "/proc/%i/fd/%i", pid, fd);
        n = readlink(proc, rpath, PATH_MAX);
        if (0 > n)
            fail("readlink failed: %s", strerror(errno));
        proc[n] = '\0';

        if (0 != strncmp(rpath, "/dev/null", 10))
            fail("expected /dev/null got \"%s\"", rpath);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_wronly_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_rdwr_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDWR);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval,
                "Failed to set retval to EPERM (got %d)", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(check_syscall_check_open_rdwr_predict) {
    pid_t pid;
    int pfd[2];
    char cwd[PATH_MAX];
    char *rcwd;

    /* setup */
    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    if (0 > pipe(pfd))
        fail("pipe() failed: %s", strerror(errno));
    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", open("emily/syd.txt", O_RDWR | O_CREAT));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        close(pfd[1]);

        pathnode_new(&(ctx->predict_prefixes), rcwd);
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_unless(0 == ptrace(PTRACE_CONT, pid, NULL, NULL),
                "PTRACE_CONT failed: %s", strerror(errno));

        int fd, n;
        char buf[16], proc[PATH_MAX], rpath[PATH_MAX];

        if (0 > read(pfd[0], buf, 16))
            fail("read() failed: %s", strerror(errno));
        fd = atoi(buf);

        snprintf(proc, PATH_MAX, "/proc/%i/fd/%i", pid, fd);
        n = readlink(proc, rpath, PATH_MAX);
        if (0 > n)
            fail("readlink failed: %s", strerror(errno));
        proc[n] = '\0';

        if (0 != strncmp(rpath, "/dev/null", 10))
            fail("expected /dev/null got \"%s\"", rpath);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_rdwr_allow) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDWR);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_magic_write) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open(CMD_WRITE"/var/empty", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected deny");

        fail_if(0 == pathlist_check(&(ctx->write_prefixes), "/var/empty"),
                "Pathlist check failed for /var/empty, expected success");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_open_magic_predict) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        open(CMD_PREDICT"/var/empty", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->predict_prefixes), "/home/emily:/dev:/tmp");
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected deny");

        fail_if(0 == pathlist_check(&(ctx->predict_prefixes), "/var/empty"),
                "Pathlist check failed for /var/empty, expected success");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_creat_deny) {
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        creat("emily/syd.txt", 0644);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_if(syscall_check(ctx, ctx->eldest, syscall),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval,
                "Failed to set retval to EPERM (got %d)", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_creat_predict) {
    pid_t pid;
    int pfd[2];
    char cwd[PATH_MAX];
    char *rcwd;

    /* setup */
    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    if (0 > pipe(pfd))
        fail("pipe() failed: %s", strerror(errno));
    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", creat("emily/syd.txt", 0644));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        context_t *ctx = context_new();

        close(pfd[1]);

        pathnode_new(&(ctx->predict_prefixes), rcwd);
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_unless(0 == ptrace(PTRACE_CONT, pid, NULL, NULL),
                "PTRACE_CONT failed: %s", strerror(errno));

        int fd, n;
        char buf[16], proc[PATH_MAX], rpath[PATH_MAX];

        if (0 > read(pfd[0], buf, 16))
            fail("read() failed: %s", strerror(errno));
        fd = atoi(buf);

        snprintf(proc, PATH_MAX, "/proc/%i/fd/%i", pid, fd);
        n = readlink(proc, rpath, PATH_MAX);
        if (0 > n)
            fail("readlink failed: %s", strerror(errno));
        proc[n] = '\0';

        if (0 != strncmp(rpath, "/dev/null", 10))
            fail("expected /dev/null got \"%s\"", rpath);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(check_syscall_check_creat_allow) {
    pid_t pid;
    char cwd[PATH_MAX];
    char *rcwd;

    /* setup */
    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        creat("emily/syd.txt", 0644);
        pause();
    }
    else { /* parent */
        int status;
        long syscall;
        struct stat buf;
        context_t *ctx = context_new();

        pathlist_init(&(ctx->write_prefixes), rcwd);
        tchild_new(&(ctx->children), pid);
        ctx->eldest = ctx->children;

        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGSTOP",
                pid);
        fail_unless(0 == trace_setup(pid),
                "Failed to set tracing options: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_unless(0 == ptrace(PTRACE_SYSCALL, pid, NULL, NULL),
                "PTRACE_SYSCALL failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status),
                "child %i didn't stop by sending itself SIGTRAP",
                pid);

        fail_if(0 > trace_get_syscall(pid, &syscall),
                "Failed to get syscall: %s", strerror(errno));
        fail_unless(syscall_check(ctx, ctx->eldest, syscall),
                "Denied access, expected allow");
        fail_unless(0 > stat("emily/syd.txt", &buf),
                "Allowed access but file doesn't exist: %s", strerror(errno));
        kill(pid, SIGTERM);
    }
}
END_TEST

Suite *syscall_suite_create(void) {
    Suite *s = suite_create("syscall");

    /* syscall_check test cases */
    TCase *tc_syscall_check = tcase_create("syscall_check");
    tcase_add_checked_fixture(tc_syscall_check, syscall_setup, syscall_teardown);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_chmod_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_chown_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_chown_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_chown_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_rdonly_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_wronly_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_wronly_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_wronly_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_rdwr_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_rdwr_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_rdwr_allow);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_magic_write);
    tcase_add_test(tc_syscall_check, check_syscall_check_open_magic_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_creat_deny);
    tcase_add_test(tc_syscall_check, check_syscall_check_creat_predict);
    tcase_add_test(tc_syscall_check, check_syscall_check_creat_allow);
    suite_add_tcase(s, tc_syscall_check);

    return s;
}
