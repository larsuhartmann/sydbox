#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

clean_files+=( "arnold.layne.fifo" )

start_test "t08-mknod-deny"
sydbox -- ./t08_mknod
if [[ 0 == $? ]]; then
    die "failed to deny mknod"
elif [[ -p arnold.layne.fifo ]]; then
    die "fifo exists, failed to deny mknod"
fi
end_test

start_test "t08-mknod-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t08_mknod
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -p arnold.layne.fifo ]]; then
    die "fifo doesn't exist, write didn't allow access"
fi
end_test

# Tests dealing with too long paths
fname="arnold.layne.fifo"
mkdir_long

start_test "t08-mknod-toolong-deny"
sydbox -- ./t08_mknod_toolong "$long_dir" "$fname"
if [[ 0 == $? ]]; then
    die "failed to deny mknod"
elif lstat_long "$fname"; then
    die "failed to deny mknod, fifo exists"
fi

start_test "t08-mknod-toolong-write"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t08_mknod_toolong "$long_dir" "$fname"
if [[ 0 != $? ]]; then
    die "failed to allow mknod"
elif ! lstat_long "$fname"; then
    die "failed to allow mknod, fifo doesn't exist"
fi
end_test
