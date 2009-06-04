#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "jugband.blues" )

# To make sure symbolic links are handled correctly, add see.emily.play to
# SANDBOX_WRITE as jugband.blues will be a symbolic link to a file in that directory.
export SANDBOX_WRITE="$cwd"/see.emily.play

start_test "t29-symlinkat-deny"
sydbox -- ./t29_symlinkat
if [[ 0 == $? ]]; then
    die "failed to deny symlinkat"
elif [[ -h jugband.blues ]]; then
    die "symlink exists, failed to deny symlinkat"
fi
end_test

start_test "t29-symlinkat-predict"
SANDBOX_PREDICT="$cwd" sydbox -- ./t29_symlinkat
if [[ 0 != $? ]]; then
    die "failed to predict symlinkat"
elif [[ -h jugband.blues ]]; then
    die "predict allowed access"
fi
end_test

start_test "t29-symlinkat-write"
SANDBOX_WRITE="$cwd" sydbox -- ./t29_symlinkat
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -h jugband.blues ]]; then
    die "symlink doesn't exist, write didn't allow access"
fi
end_test
