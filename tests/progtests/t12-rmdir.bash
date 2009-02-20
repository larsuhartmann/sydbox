#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t12-rmdir-deny"
sydbox -- ./t12_rmdir
if [[ 0 == $? ]]; then
    die "failed to deny rmdir"
elif [[ ! -d see.emily.play ]]; then
    die "dir doesn't exist, failed to deny rmdir"
fi
end_test

start_test "t12-rmdir-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t12_rmdir
if [[ 0 != $? ]]; then
    die "failed to predict rmdir"
elif [[ ! -d see.emily.play ]]; then
    die "predict allowed access"
fi
end_test

start_test "t12-rmdir-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t12_rmdir
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ -d see.emily.play ]]; then
    die "dir exists, write didn't allow access"
fi
end_test
