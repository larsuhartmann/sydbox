#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "jugband.blues" )

# To make sure symbolic links are handled correctly, add see.emily.play to
# SYDBOX_WRITE as jugband.blues will be a symbolic link to a file in that directory.
export SYDBOX_WRITE="$cwd"/see.emily.play

start_test "t28-symlinkat-atfdcwd-deny"
sydbox -- ./t28_symlinkat_atfdcwd
if [[ 0 == $? ]]; then
    die "failed to deny symlinkat"
elif [[ -h jugband.blues ]]; then
    die "symlink exists, failed to deny symlinkat"
fi
end_test

start_test "t28-symlinkat-atfdcwd-write"
SYDBOX_WRITE="$cwd" sydbox -- ./t28_symlinkat_atfdcwd
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -h jugband.blues ]]; then
    die "symlink doesn't exist, write didn't allow access"
fi
end_test
