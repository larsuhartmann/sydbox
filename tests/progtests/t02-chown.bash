#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t02-chown-deny"
sydbox -- ./t02_chown
if [[ 0 == $? ]]; then
    die "failed to deny chown"
fi
end_test

start_test "t02-chown-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t02_chown
if [[ 0 != $? ]]; then
    die "failed to predict chown"
fi
end_test

start_test "t02-chown-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t02_chown
if [[ 0 != $? ]]; then
    die "write didn't allow access"
fi
end_test

# Tests dealing with too long paths
tmpfile="$(mkstemp_long)"

start_test "t02-chown-deny-toolong"
sydbox -- ./t02_chown_toolong "$long_dir" "$tmpfile"
if [[ 0 == $? ]]; then
    die "failed to deny chown"
fi
end_test

start_test "t02-chown-predict-toolong"
SANDBOX_PREDICT="$cwd"/$long_dir sydbox -- ./t02_chown_toolong "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to predict chown"
fi
end_test

start_test "t02-chown-allow-toolong"
SANDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t02_chown_toolong "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "write didn't allow access"
fi
end_test
