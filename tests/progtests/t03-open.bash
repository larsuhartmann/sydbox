#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

start_test "t03-open-rdonly-allow"
sydbox -- ./t03_open 0
if [[ 0 != $? ]]; then
    die "denied access for open(\"arnold.layne\", O_RDONLY)"
fi
end_test

start_test "t03-open-wronly-deny"
sydbox -- ./t03_open 1
if [[ 0 == $? ]]; then
    die "allowed access for open(\"arnold.layne\", O_WRONLY)"
fi
end_test

start_test "t03-open-wronly-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t03_open 1
if [[ 0 != $? ]]; then
    die "failed to predict open(\"arnold.layne\", O_WRONLY)"
fi
if [[ ! -z "$(<arnold.layne)" ]]; then
    die "predict allowed access to O_WRONLY"
fi
end_test

start_test "t03-open-wronly-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t03_open 1
if [[ 0 != $? ]]; then
    die "failed to allow open(\"arnold.layne\", O_WRONLY)"
fi
if [[ -z "$(<arnold.layne)" ]]; then
    die "failed to write to file with O_WRONLY"
fi
end_test

start_test "t03-open-rdwr-deny"
sydbox -- ./t03_open 2
if [[ 0 == $? ]]; then
    die "allowed access for open(\"arnold.layne\", O_RDWR)"
fi
end_test

:>arnold.layne
start_test "t03-open-rdwr-predict"
SANDBOX_PREDICT="${cwd}" sydbox -- ./t03_open 2
if [[ 0 != $? ]]; then
    die "failed to predict open(\"arnold.layne\", O_RDWR)"
fi
if [[ ! -z "$(<arnold.layne)" ]]; then
    die "predict allowed access to O_RDWR"
fi
end_test

start_test "t03-open-rdwr-write"
SANDBOX_WRITE="${cwd}" sydbox -- ./t03_open 2
if [[ 0 != $? ]]; then
    die "failed to allow open(\"arnold.layne\", O_RDWR)"
fi
if [[ -z "$(<arnold.layne)" ]]; then
    die "failed to write to file with O_RDWR"
fi
end_test

# Tests dealing with too long paths
tmpfile="$(mkstemp_long)"

start_test "t03-open-rdonly-toolong-allow"
sydbox -- ./t03_open_toolong 0 "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "denied access for open(\"TOO_LONG_DIR/$tmpfile\", O_RDONLY)"
fi
end_test

start_test "t03-open-wronly-toolong-deny"
sydbox -- ./t03_open_toolong 1 "$long_dir" "$tmpfile"
if [[ 0 == $? ]]; then
    die "allowed access for open(\"TOO_LONG_DIR/$tmpfile\", O_WRONLY)"
fi
end_test

start_test "t03-open-wronly-toolong-predict"
SANDBOX_PREDICT="$cwd"/$long_dir sydbox -- ./t03_open_toolong 1 "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to predict open(\"TOO_LONG_DIR/$tmpfile\", O_WRONLY)"
fi
data="$(read_long $tmpfile)"
if [[ ! -z "$data" ]]; then
    die "predict allowed access to O_WRONLY"
fi
end_test

start_test "t03-open-wronly-toolong-write"
SANDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t03_open_toolong 1 "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to allow open(\"TOO_LONG_DIR/$tmpfile\", O_WRONLY)"
fi
data="$(read_long $tmpfile)"
if [[ -z "$data" ]]; then
    die "failed to allow access to O_WRONLY"
fi
end_test

start_test "t03-open-rdwr-toolong-deny"
sydbox -- ./t03_open_toolong 2 "$long_dir" "$tmpfile"
if [[ 0 == $? ]]; then
    die "allowed access for open(\"TOO_LONG_DIR/$tmpfile\", O_RDWR)"
fi
end_test

# Need to cleanup here because the file has already been written
rm -fr "$long_dir"
tmpfile="$(mkstemp_long)"

start_test "t03-open-rdwr-toolong-predict"
SANDBOX_PREDICT="$cwd"/$long_dir sydbox -- ./t03_open_toolong 2 "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to predict open(\"TOO_LONG_DIR/$tmpfile\", O_RDWR)"
fi
data="$(read_long $tmpfile)"
if [[ ! -z "$data" ]]; then
    die "predict allowed access to O_RDWR"
fi
end_test

start_test "t03-open-rdwr-toolong-write"
SANDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t03_open_toolong 2 "$long_dir" "$tmpfile"
if [[ 0 != $? ]]; then
    die "failed to allow open(\"TOO_LONG_DIR/$tmpfile\", O_RDWR)"
fi
data="$(read_long $tmpfile)"
if [[ -z "$data" ]]; then
    die "failed to write to file with O_RDWR"
fi
end_test
