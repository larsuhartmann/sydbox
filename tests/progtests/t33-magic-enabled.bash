#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t33-magic-enabled-on"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/enabled ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/enabled doesn't exist when sydbox is on"
fi
end_test

start_test "t33-magic-enabled-off"
sydbox -- bash <<EOF
:>/dev/sydbox/off
[[ -e /dev/sydbox/enabled ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/enabled exists when sydbox is off"
fi
end_test

start_test "t33-magic-enabled-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/enabled ]]
EOF
if [[ 0 == $? ]]; then
    die "failed to lock /dev/sydbox/enabled"
fi
end_test
