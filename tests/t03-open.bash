#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

say "t03-open-rdonly-allow"
sydbox -- ./t03_open 0
if [[ 0 != $? ]]; then
    die "denied access for open(\"arnold.layne\", O_RDONLY)"
fi

say "t03-open-wronly-deny"
sydbox -- ./t03_open 1
if [[ 0 == $? ]]; then
    die "allowed access for open(\"arnold.layne\", O_WRONLY)"
fi

say "t03-open-wronly-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t03_open 1
if [[ 0 != $? ]]; then
    die "failed to predict open(\"arnold.layne\", O_WRONLY)"
fi
if [[ ! -z "$(<arnold.layne)" ]]; then
    die "predict allowed access to O_WRONLY"
fi

say "t03-open-wronly-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t03_open 1
if [[ 0 != $? ]]; then
    die "failed to allow open(\"arnold.layne\", O_WRONLY)"
fi
if [[ -z "$(<arnold.layne)" ]]; then
    die "failed to write to file with O_WRONLY"
fi

say "t03-open-rdwr-deny"
sydbox -- ./t03_open 2
if [[ 0 == $? ]]; then
    die "allowed access for open(\"arnold.layne\", O_RDWR)"
fi

:>arnold.layne
say "t03-open-rdwr-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t03_open 2
if [[ 0 != $? ]]; then
    die "failed to predict open(\"arnold.layne\", O_RDWR)"
fi
if [[ ! -z "$(<arnold.layne)" ]]; then
    die "predict allowed access to O_RDWR"
fi

say "t03-open-rdwr-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t03_open 2
if [[ 0 != $? ]]; then
    die "failed to allow open(\"arnold.layne\", O_RDWR)"
fi
if [[ -z "$(<arnold.layne)" ]]; then
    die "failed to write to file with O_RDWR"
fi
