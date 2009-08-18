#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

. test-lib.bash

cat <<EOF >arnold.layne
Arnold Layne had a strange hobby
Collecting clothes
Moonshine washing line
They suit him fine

On the wall hung a tall mirror
Distorted view, see through baby blue
He dug it
Oh, Arnold Layne
It's not the same, takes two to know 
Two to know, two to know, two to know
Why can't you see?

Arnold Layne, Arnold Layne, Arnold Layne, Arnold Layne

Now he's caught - a nasty sort of person.
They gave him time
Doors bang - chain gang - he hates it

Oh, Arnold Layne
It's not the same, takes two to know
two to know, two to know, two to know,
Why can't you see?

Arnold Layne, Arnold Layne, Arnold Layne, Arnold Layne
Don't do it again
EOF

start_test "t14-truncate-deny"
sydbox -- ./t14_truncate
if [[ 0 == $? ]]; then
    die "failed to deny truncate"
elif [[ -z "$(<arnold.layne)" ]]; then
    die "file truncated, failed to deny truncate"
fi
end_test

start_test "t14-truncate-predict"
SYDBOX_PREDICT="${cwd}" sydbox -- ./t14_truncate
if [[ 0 != $? ]]; then
    die "failed to predict truncate"
elif [[ -z "$(<arnold.layne)" ]]; then
    die "predict allowed access"
fi
end_test

start_test "t14-truncate-write"
SYDBOX_WRITE="${cwd}" sydbox -- ./t14_truncate
if [[ 0 != $? ]]; then
    die "failed to allow access"
elif [[ ! -z "$(<arnold.layne)" ]]; then
    die "file not truncated, failed to allow access"
fi
end_test

# Tests dealing with too long paths
fname="lucifer.sam"
write_long "$fname" "That cat's something i can't explain"

start_test "t14-truncate-toolong-deny"
sydbox -- ./t14_truncate_toolong "$long_dir" "$fname"
if [[ 0 == $? ]]; then
    die "failed to deny truncate"
fi
data="$(read_long $fname)"
if [[ -z "$data" ]]; then
    die "file truncated, failed to deny truncate"
fi
end_test

start_test "t14-truncate-toolong-predict"
SYDBOX_PREDICT="$cwd"/$long_dir sydbox -- ./t14_truncate_toolong "$long_dir" "$fname"
if [[ 0 != $? ]]; then
    die "failed to predict truncate"
fi
data="$(read_long $fname)"
if [[ -z "$data" ]]; then
    die "predict allowed access"
fi
end_test

start_test "t14-truncate-toolong-write"
SYDBOX_WRITE="$cwd"/$long_dir sydbox -- ./t14_truncate_toolong "$long_dir" "$fname"
if [[ 0 != $? ]]; then
    die "failed to allow access"
fi
data="$(read_long $fname)"
if [[ ! -z "$data" ]]; then
    die "file not truncated, failed to allow access"
fi
end_test
