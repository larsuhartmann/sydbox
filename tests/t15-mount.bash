#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

old_umask=$(umask)
umask 0222 && mkdir see.emily.play && umask $old_umask
if [[ 0 != $? ]]; then
    die "mkdir see.emily.play"
fi

start_test "t15-mount-deny"
sydbox -- ./t15_mount
if [[ 0 == $? ]]; then
    die "failed to deny mount"
fi
end_test

start_test "t15-mount-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t15_mount
if [[ 0 != $? ]]; then
    die "failed to predict mount"
fi
end_test
