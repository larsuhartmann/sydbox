#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

no_create_files=1
. test-lib.bash

start_test "t40-magic-ban_exec-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/ban_exec ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/ban_exec doesn't exist"
fi
end_test

start_test "t40-magic-ban_exec-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/ban_exec ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/ban_exec exists"
fi
end_test

start_test "t40-magic-unban_exec-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/unban_exec ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/unban_exec doesn't exist"
fi
end_test

start_test "t40-magic-unban_exec-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/unban_exec ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/unban_exec exists"
fi
end_test

start_test "t40-magic-ban_exec-ban"
sydbox -- bash <<EOF
:>/dev/sydbox/ban_exec
/bin/true
EOF
if [[ 0 == $? ]]; then
    die "failed to ban execve()"
fi
end_test

start_test "t40-magic-unban_exec-unban"
sydbox -- bash <<EOF
:>/dev/sydbox/ban_exec
:>/dev/sydbox/unban_exec
/bin/true
EOF
if [[ 0 != $? ]]; then
    die "failed to unban execve()"
fi
end_test
