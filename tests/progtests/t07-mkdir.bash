#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

clean_files+=( "see.emily.play" )

start_test "t07-mkdir-deny"
sydbox -- ./t07_mkdir
if [[ 0 == $? ]]; then
    die "failed to deny mkdir"
elif [[ -d see.emily.play ]]; then
    die "dir exists, failed to deny mkdir"
fi
end_test

start_test "t07-mkdir-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t07_mkdir
if [[ 0 != $? ]]; then
    die "failed to predict mkdir"
elif [[ -d see.emily.play ]]; then
    die "predict allowed access"
fi
end_test

start_test "t07-mkdir-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t07_mkdir
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -d see.emily.play ]]; then
    die "dir doesn't exist, write didn't allow access"
fi
end_test

# Tests dealing with too long paths
dname="see.emily.play"
tmpfile="$(mkdir_long)"

start_test "t07-mkdir-toolong-deny"
sydbox -- ./t07_mkdir_toolong "$long_dir" "$dname"
if [[ 0 == $? ]]; then
    die "failed to deny mkdir"
elif stat_long "$dname"; then
    die "failed to deny mkdir, dir exists"
fi

start_test "t07-mkdir-toolong-predict"
SANDBOX_PREDICT="$cwd"/$long_dir sydbox -- ./t07_mkdir_toolong "$long_dir" "$dname"
if [[ 0 != $? ]]; then
    die "failed to predict mkdir"
elif stat_long "$dname"; then
    die "predict allowed access"
fi

start_test "t07-mkdir-toolong-write"
SANDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t07_mkdir_toolong "$long_dir" "$dname"
if [[ 0 != $? ]]; then
    die "failed to allow mkdir"
elif ! stat_long "$dname"; then
    die "failed to allow mkdir, dir doesn't exist"
fi
end_test
