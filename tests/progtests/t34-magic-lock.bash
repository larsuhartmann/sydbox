#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t34-magic-lock-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/lock ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/lock doesn't exist"
fi
end_test

start_test "t34-magic-lock-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/lock ]]
EOF
if [[ 0 == $? ]]; then
    die "failed to lock /dev/sydbox/lock"
fi
end_test

start_test "t34-magic-lock-disable"
sydbox -- bash <<EOF
:>/dev/sydbox/lock
[[ -e /dev/sydbox ]]
EOF
if [[ 0 == $? ]]; then
    die "locking failed"
fi
end_test
