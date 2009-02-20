#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t11-rename-second-deny"
SANDBOX_WRITE="${cwd}" sydbox -- ./t11_rename_second
if [[ 0 == $? ]]; then
    die "failed to deny rename"
elif [[ -f /tmp/sydbox.txt ]]; then
    die "file exists, failed to deny rename"
fi
end_test

start_test "t11-rename-second-predict"
SANDBOX_WRITE="${cwd}" SANDBOX_PREDICT="/tmp" sydbox -- ./t11_rename_second
if [[ 0 != $? ]]; then
    die "failed to predict rename"
elif [[ -f /tmp/sydbox.txt ]]; then
    die "predict allowed access"
fi
end_test

start_test "t11-rename-second-write"
SANDBOX_WRITE="${cwd}:/tmp" sydbox -- ./t11_rename_second
if [[ 0 != $? ]]; then
    die "failed to allow rename"
elif [[ ! -f /tmp/sydbox.txt ]]; then
    die "file doesn't exist, failed to allow rename"
fi
end_test
