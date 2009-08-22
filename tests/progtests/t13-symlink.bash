#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "jugband.blues" )

start_test "t13-symlink-deny"
sydbox -- ./t13_symlink
if [[ 0 == $? ]]; then
    die "failed to deny symlink"
elif [[ -h jugband.blues ]]; then
    die "symlink exists, failed to deny symlink"
fi
end_test

start_test "t13-symlink-deny"
SYDBOX_WRITE="${cwd}" sydbox -- ./t13_symlink
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -h jugband.blues ]]; then
    die "symlink doesn't exist, write didn't allow access"
fi
end_test

# Tests dealing with too long paths
sname="jugband.blues"
tname="/tmp/arnold.layne"
mkdir_long

# Make sure symlinks are handled correctly
export SYDBOX_WRITE=/tmp

start_test "t13-symlink-toolong-deny"
sydbox -- ./t13_symlink_toolong "$long_dir" "$tname" "$sname"
if [[ 0 == $? ]]; then
    die "failed to deny symlink"
elif lstat_long "$sname" >>"${SYDBOX_LOG}" 2>&1; then
    die "symlink exists, failed to deny symlink"
fi
end_test

start_test "t13-symlink-toolong-write"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t13_symlink_toolong "$long_dir" "$tname" "$sname"
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif ! lstat_long "$sname" >>"${SYDBOX_LOG}" 2>&1; then
    die "symlink doesn't exist, write didn't allow access"
fi
end_test
