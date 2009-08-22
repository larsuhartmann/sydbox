#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t35-magic-exec_lock-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/exec_lock ]]
EOF
if [[ 0 == $? ]]; then
    die "failed to lock /dev/sydbox/exec_lock"
fi
end_test

start_test "t35-magic-exec_lock-disable-on-exec"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/exec_lock ]]
bash -c "[[ -e /dev/sydbox ]]"
EOF
if [[ 0 == $? ]]; then
    die "exec_lock failed to lock after execve()"
fi
end_test
