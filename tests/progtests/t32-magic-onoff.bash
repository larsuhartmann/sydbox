#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t32-magic-onoff-set-on"
sydbox -- ./t32_magic_onoff_set_on
if [[ 0 != $? ]]; then
    die "failed to set sydbox on"
fi
end_test

start_test "t32-magic-onoff-set-on-locked"
sydbox --lock -- ./t32_magic_onoff_set_on
if [[ 0 == $? ]]; then
    die "failed to lock /dev/sydbox/on"
fi
end_test

start_test "t32-magic-onoff-set-off"
sydbox -- ./t32_magic_onoff_set_off
if [[ 0 != $? ]]; then
    die "failed to set sydbox off"
fi
end_test

start_test "t32-magic-onoff-set-off-locked"
sydbox --lock -- ./t32_magic_onoff_set_off
if [[ 0 == $? ]]; then
    die "failed to lock /dev/sydbox/off"
fi
end_test

start_test "t32-magic-onoff-check-off"
sydbox -- ./t32_magic_onoff_check_off
if [[ 0 != $? ]]; then
    die "/dev/sydbox/off check failed"
elif [[ -z "$(< arnold.layne)" ]]; then
    die "file empty, failed to set sydbox off"
fi
end_test

:>arnold.layne
if [[ -n "$(< arnold.layne)" ]]; then
    say "skip" "failed to truncate arnold.layne, skipping following tests"
    exit 0
fi

start_test "t32-magic-onoff-check-on"
sydbox -- ./t32_magic_onoff_check_on
if [[ 0 != $? ]]; then
    die "/dev/sydbox/on check failed"
elif [[ -n "$(< arnold.layne)" ]]; then
    die "file not empty, failed to set sydbox on"
fi
end_test
