#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t01-chmod-deny"
sydbox -- ./t01_chmod
if [[ 0 == $? ]]; then
    die "failed to deny chmod"
fi
end_test

start_test "t01-chmod-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t01_chmod
if [[ 0 != $? ]]; then
    die "failed to predict chmod"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "${perms}" != '-rw-r--r--' ]]; then
    die "predict allowed access"
fi
end_test

start_test "t01-chmod-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t01_chmod
if [[ 0 != $? ]]; then
    die "failed to allow chmod"
fi
perms=$(ls -l arnold.layne | cut -d' ' -f1)
if [[ "${perms}" != '----------' ]]; then
    die "write didn't allow access"
fi
end_test
