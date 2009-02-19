#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t17-umount2-deny"
sydbox -- ./t17_umount2
if [[ 0 == $? ]]; then
    die "failed to deny umount2"
fi
end_test

start_test "t17-umount2-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t17_umount2
if [[ 0 != $? ]]; then
    die "failed to predict umount2"
fi
end_test
