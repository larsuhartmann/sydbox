#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t19-unlink-deny"
sydbox -- ./t19_unlink
if [[ 0 == $? ]]; then
    die "failed to deny unlink"
elif [[ ! -f arnold.layne ]]; then
    die "file doesn't exist, failed to deny unlink"
fi
end_test

start_test "t19-unlink-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t19_unlink
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ -f arnold.layne ]]; then
    die "file exists, write didn't allow access"
fi
end_test

# Tests dealing with too long paths
tmpfile="$(mkstemp_long)"

start_test "t19-unlink-toolong-deny"
sydbox -- ./t19_unlink_toolong "$long_dir" "$tmpfile"
if [[ 0 == $? ]]; then
    die "failed to deny unlink"
elif ! lstat_long "$tmpfile"; then
    die "file doesn't exist, failed to deny unlink"
fi
end_test

start_test "t19-unlink-toolong-write"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t19_unlink_toolong "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif lstat_long "$tmpfile"; then
    die "file exists, write didn't allow access"
fi
end_test
