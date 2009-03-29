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
#include "../src/path.h"
#include "../src/trace.h"
#include "../src/syscall.h"
#include "../src/children.h"
#include "check_sydbox.h"

void syscall_setup(void) {
    mkdir("emily", 0755);
    symlink("/dev/null", "arnold_layne");
}

void syscall_teardown(void) {
    unlink("arnold_layne");
    unlink("emily/syd.txt");
    rmdir("emily");
}

START_TEST(syscall_check_chmod_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_chmod_predict) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->predict_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Allowed access, expected deny");
        fail_unless(0 == ctx->eldest->retval, "Expected 0 got %d", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_chmod_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chmod("/dev/null", 0755);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_chown_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_chown_predict) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->predict_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Allowed access, expected deny");
        fail_unless(0 == ctx->eldest->retval, "Expected 0 got %d", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_chown_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        chown("/dev/null", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_open_rdonly_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_open_wronly_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(syscall_check_open_wronly_predict) {
    PRINT_TEST_HEADER;
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
        trace_me();
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", open("emily/syd.txt", O_WRONLY | O_CREAT));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long sno;

        context_t *ctx = context_new();

        close(pfd[1]);

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathnode_new(&(ctx->eldest->sandbox->predict_prefixes), rcwd, 1);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));

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

START_TEST(syscall_check_open_wronly_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_open_rdwr_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDWR);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);

        kill(pid, SIGTERM);
    }

}
END_TEST

START_TEST(syscall_check_open_rdwr_predict) {
    PRINT_TEST_HEADER;
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
        trace_me();
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", open("emily/syd.txt", O_RDWR | O_CREAT));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        close(pfd[1]);

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathnode_new(&(ctx->eldest->sandbox->predict_prefixes), rcwd, 1);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));

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

START_TEST(syscall_check_open_rdwr_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open("/dev/null", O_RDWR);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_open_magic_write) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open(CMD_WRITE"/var/empty", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        fail_if(0 == pathlist_check(ctx->eldest->sandbox->write_prefixes, "/var/empty"),
                "Pathlist check failed for /var/empty, expected success");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_open_magic_predict) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        open(CMD_PREDICT"/var/empty", O_WRONLY);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->predict_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        fail_if(0 == pathlist_check(ctx->eldest->sandbox->predict_prefixes, "/var/empty"),
                "Pathlist check failed for /var/empty, expected success");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_creat_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        creat("emily/syd.txt", 0644);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_creat_predict) {
    PRINT_TEST_HEADER;
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
        trace_me();
        kill(getpid(), SIGSTOP);

        char buf[16];
        snprintf(buf, 16, "%d", creat("emily/syd.txt", 0644));
        write(pfd[1], buf, 16);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        close(pfd[1]);

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathnode_new(&(ctx->eldest->sandbox->predict_prefixes), rcwd, 1);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        /* Resume the child so it writes to the pipe */
        fail_if(0 > trace_cont(pid), "trace_cont() failed: %s", strerror(errno));

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

START_TEST(syscall_check_creat_allow) {
    PRINT_TEST_HEADER;
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
        trace_me();
        kill(getpid(), SIGSTOP);
        creat("emily/syd.txt", 0644);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        struct stat buf;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), rcwd);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");
        fail_unless(0 > stat("emily/syd.txt", &buf), "Allowed access but file doesn't exist: %s",
                strerror(errno));
        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_stat_magic) {
    PRINT_TEST_HEADER;
    pid_t pid;
    struct stat buf;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { // child
        trace_me();
        kill(getpid(), SIGSTOP);
        stat(CMD_PATH, &buf);
        pause();
    }
    else { // parent
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_stat_magic_write) {
    PRINT_TEST_HEADER;
    pid_t pid;
    struct stat buf;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { // child
        trace_me();
        kill(getpid(), SIGSTOP);
        stat(CMD_WRITE, &buf);
        pause();
    }
    else { // parent
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_stat_magic_predict) {
    PRINT_TEST_HEADER;
    pid_t pid;
    struct stat buf;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { // child
        trace_me();
        kill(getpid(), SIGSTOP);
        stat(CMD_PREDICT, &buf);
        pause();
    }
    else { // parent
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_lchown_deny) {
    PRINT_TEST_HEADER;
    pid_t pid;

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        lchown("arnold_layne", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathlist_init(&(ctx->eldest->sandbox->write_prefixes), "/dev:/tmp");

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno),
                "Allowed access, expected violation");
        fail_unless(-EPERM == ctx->eldest->retval, "Failed to set retval to EPERM (got %d)",
                ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_lchown_predict) {
    PRINT_TEST_HEADER;
    pid_t pid;
    char cwd[PATH_MAX];
    char *rcwd;

    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        lchown("arnold_layne", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathnode_new(&(ctx->eldest->sandbox->predict_prefixes), rcwd, 1);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_DENY == syscall_check(ctx, ctx->eldest, sno), "Allowed access, expected deny");
        fail_unless(0 == ctx->eldest->retval, "Expected 0 got %d", ctx->eldest->retval);

        kill(pid, SIGTERM);
    }
}
END_TEST

START_TEST(syscall_check_lchown_allow) {
    PRINT_TEST_HEADER;
    pid_t pid;
    char cwd[PATH_MAX];
    char *rcwd;

    if (NULL == getcwd(cwd, PATH_MAX))
        fail("getcwd failed: %s", strerror(errno));
    rcwd = realpath(cwd, NULL);
    if (NULL == rcwd)
        fail("realpath failed: %s", strerror(errno));

    pid = fork();
    if (0 > pid)
        fail("fork() failed: %s", strerror(errno));
    else if (0 == pid) { /* child */
        trace_me();
        kill(getpid(), SIGSTOP);
        lchown("arnold_layne", 0, 0);
        pause();
    }
    else { /* parent */
        int status;
        long sno;
        context_t *ctx = context_new();

        tchild_new(&(ctx->children), pid);
        ctx->eldest = (struct tchild *) ctx->children->data;
        pathnode_new(&(ctx->eldest->sandbox->write_prefixes), rcwd, 1);

        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGSTOP", pid);
        fail_unless(0 == trace_setup(pid), "Failed to set tracing options: %s", strerror(errno));
        ctx->eldest->cwd = getcwd(NULL, PATH_MAX);
        fail_if(NULL == ctx->eldest->cwd, "Failed to get cwd: %s", strerror(errno));

        /* Resume the child, it will stop at the next system call. */
        fail_if(0 > trace_syscall(pid, 0), "trace_syscall() failed: %s", strerror(errno));
        wait(&status);
        fail_unless(WIFSTOPPED(status), "child %i didn't stop by sending itself SIGTRAP", pid);

        fail_if(0 > trace_get_syscall(pid, &sno), "Failed to get syscall: %s", strerror(errno));
        fail_unless(RS_ALLOW == syscall_check(ctx, ctx->eldest, sno),
                "Denied access, expected allow");

        kill(pid, SIGTERM);
    }
}
END_TEST

Suite *syscall_suite_create(void) {
    Suite *s = suite_create("syscall");

    /* syscall_check test cases */
    TCase *tc_syscall = tcase_create("syscall");
    tcase_add_checked_fixture(tc_syscall, syscall_setup, syscall_teardown);
    tcase_add_test(tc_syscall, syscall_check_chmod_deny);
    tcase_add_test(tc_syscall, syscall_check_chmod_predict);
    tcase_add_test(tc_syscall, syscall_check_chmod_allow);
    tcase_add_test(tc_syscall, syscall_check_chown_deny);
    tcase_add_test(tc_syscall, syscall_check_chown_predict);
    tcase_add_test(tc_syscall, syscall_check_chown_allow);
    tcase_add_test(tc_syscall, syscall_check_open_rdonly_allow);
    tcase_add_test(tc_syscall, syscall_check_open_wronly_deny);
    tcase_add_test(tc_syscall, syscall_check_open_wronly_predict);
    tcase_add_test(tc_syscall, syscall_check_open_wronly_allow);
    tcase_add_test(tc_syscall, syscall_check_open_rdwr_deny);
    tcase_add_test(tc_syscall, syscall_check_open_rdwr_predict);
    tcase_add_test(tc_syscall, syscall_check_open_rdwr_allow);
    tcase_add_test(tc_syscall, syscall_check_open_magic_write);
    tcase_add_test(tc_syscall, syscall_check_open_magic_predict);
    tcase_add_test(tc_syscall, syscall_check_creat_deny);
    tcase_add_test(tc_syscall, syscall_check_creat_predict);
    tcase_add_test(tc_syscall, syscall_check_creat_allow);
    tcase_add_test(tc_syscall, syscall_check_stat_magic);
    tcase_add_test(tc_syscall, syscall_check_stat_magic_write);
    tcase_add_test(tc_syscall, syscall_check_stat_magic_predict);
    tcase_add_test(tc_syscall, syscall_check_lchown_deny);
    tcase_add_test(tc_syscall, syscall_check_lchown_predict);
    tcase_add_test(tc_syscall, syscall_check_lchown_allow);
    suite_add_tcase(s, tc_syscall);

    return s;
}
