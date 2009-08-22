#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.hard" )

start_test "t27-linkat-second-deny"
SYDBOX_WRITE="${cwd}/see.emily.play" sydbox -- ./t27_linkat_second
if [[ 0 == $? ]]; then
    die "failed to deny linkat"
elif [[ -f arnold.layne.hard ]]; then
    die "file exists, failed to deny linkat"
fi
end_test

start_test "t27-linkat-second-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t27_linkat_second
if [[ 0 != $? ]]; then
    die "failed to allow linkat"
elif [[ ! -f arnold.layne.hard ]]; then
    die "file doesn't exist, failed to allow linkat"
fi
end_test
