#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "jugband.blues" )

start_test "t11-rename-second-deny"
SYDBOX_WRITE="${cwd}/see.emily.play" sydbox -- ./t11_rename_second
if [[ 0 == $? ]]; then
    die "failed to deny rename"
elif [[ -f jugband.blues ]]; then
    die "file exists, failed to deny rename"
fi
end_test

start_test "t11-rename-second-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t11_rename_second
if [[ 0 != $? ]]; then
    die "failed to allow rename"
elif [[ ! -f jugband.blues ]]; then
    die "file doesn't exist, failed to allow rename"
fi
end_test
