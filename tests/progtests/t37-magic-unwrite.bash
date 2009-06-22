#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t37-magic-unwrite-stat"
sydbox -- bash <<EOF
[[ -e /dev/sydbox/unwrite ]]
EOF
if [[ 0 != $? ]]; then
    die "/dev/sydbox/unwrite doesn't exist"
fi
end_test

start_test "t37-magic-unwrite-locked"
sydbox --lock -- bash <<EOF
[[ -e /dev/sydbox/unwrite ]]
EOF
if [[ 0 == $? ]]; then
    die "/dev/sydbox/unwrite exists"
fi
end_test

start_test "t37-magic-unwrite-remove"
sydbox -- bash <<EOF
:>/dev/sydbox/write/${cwd}
:>/dev/sydbox/unwrite/${cwd}
echo Oh Arnold Layne, its not the same > arnold.layne
EOF
if [[ 0 == $? ]]; then
    die "failed to remove prefix using /dev/sydbox/unwrite"
elif [[ -n "$(< arnold.layne)" ]]; then
    die "file not empty, failed to remove prefix using /dev/sydbox/unwrite"
fi
end_test
