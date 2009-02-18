#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

rm -fr arnold.layne
if [[ 0 != $? ]]; then
    die "rm -fr arnold.layne"
fi

start_test "t08-mknod-deny"
sydbox -- ./t08_mknod
if [[ 0 == $? ]]; then
    die "failed to deny mknod"
elif [[ -p arnold.layne ]]; then
    die "fifo exists, failed to deny mknod"
fi
end_test

start_test "t08-mknod-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t08_mknod
if [[ 0 != $? ]]; then
    die "failed to predict mknod"
elif [[ -p arnold.layne ]]; then
    die "predict allowed access"
fi
end_test

start_test "t08-mknod-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t08_mknod
if [[ 0 != $? ]]; then
    die "write didn't allow access"
elif [[ ! -p arnold.layne ]]; then
    die "fifo doesn't exist, write didn't allow access"
fi
end_test
