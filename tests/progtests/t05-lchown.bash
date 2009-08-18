#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

# To make sure symbolic links are handled correctly, add see.emily.play to
# SYDBOX_WRITE as its.not.the.same is a symbolic link to a file in that directory.
export SYDBOX_WRITE="$cwd"/see.emily.play

start_test "t05-lchown-deny"
sydbox -- ./t05_lchown
if [[ 0 == $? ]]; then
    die "failed to deny lchown"
fi
end_test

start_test "t05-lchown-predict"
SYDBOX_PREDICT="${cwd}" sydbox -- ./t05_lchown
if [[ 0 != $? ]]; then
    die "failed to predict lchown"
fi
end_test

start_test "t05-lchown-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t05_lchown
if [[ 0 != $? ]]; then
    die "write didn't allow access"
fi
end_test
