#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t31-fchmodat-deny"
sydbox -- ./t31_fchmodat
if [[ 0 == $? ]]; then
    die "failed to deny fchmodat"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "$perms" != '-rw-r--r--' ]]; then
    die "permissions changed, failed to deny fchmodat"
fi
end_test

start_test "t31-fchmodat-predict"
SANDBOX_PREDICT="$cwd" sydbox -- ./t31_fchmodat
if [[ 0 != $? ]]; then
    die "failed to predict fchmodat"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "$perms" != '-rw-r--r--' ]]; then
    die "predict allowed access"
fi
end_test

start_test "t31-fchmodat-write"
SANDBOX_WRITE="$cwd" sydbox -- ./t31_fchmodat
if [[ 0 != $? ]]; then
    die "failed to allow fchmodat"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "$perms" != '----------' ]]; then
    die "permissions haven't changed, failed to allow fchmodat"
fi
end_test
