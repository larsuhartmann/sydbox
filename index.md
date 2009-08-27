---
layout: default
title: Sydbox the other sandbox
---

## About ##

- [Ptrace](http://linux.die.net/man/2/ptrace) based sandbox implementation.
- Intercepts system calls and checks for allowed filesystem prefixes, denies them when checks fail.
- Basic support to disallow network connections.
- Basic support to sandbox [execve(2)](http://linux.die.net/man/2/execve) calls.
- Based in part upon [catbox](https://svn.uludag.org.tr/uludag/trunk/python-modules/catbox/) and
  [strace](http://sourceforge.net/projects/strace).
- Distributed under the terms of the [GPL2](http://www.gnu.org/licenses/gpl-2.0.html).

## Why ##

- [Gentoo](http://www.gentoo.org)'s sandbox sucks for several reasons,
  [Exherbo](http://www.exherbo.org) needs a better one.

## Bugs/Support ##

- Join our channel [#sydbox](irc://irc.freenode.net/sydbox) on [freenode](http://freenode.net/).

## Contribute ##

- Clone [git://github.com/alip/sydbox.git](git://github.com/alip/sydbox.git).
- Read [TODO](http://github.com/alip/sydbox/blob/master/TODO.mkd) and see open [bugs](http://bit.ly/MzeIv).
- Format patches are preferred. Either mail us or poke us on IRC.

## FAQ ##

- Why am I getting an access violation in my build directory?

  This can happen if the **$PALUDIS\_TMPDIR** environment variable is beneath  
  a symlinked directory or is a [symbolic link](http://en.wikipedia.org/wiki/Symbolic_link) itself,  
  since `mkdir()` resolves symbolic links and so does sydbox.

- Why is sydbox so slow?

  This is probably because you're running a kernel version older than 2.6.29 which had a ptrace bug.  
  To fix the issue either upgrade your kernel to a recent version or apply
  [this](http://git.kernel.org/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=53da1d9456fe7f87a920a78fdbdcf1225d197cb7).

- I upgraded my kernel but sydbox is still slow, why?

  This is because the program you're running under sydbox is either creating a very long directory  
  structure or it's creating lots of simultaneous threads. Try decreasing the number of simultaneous  
  threads.

<!-- vim: set tw=100 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
