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

- Gentoo's sandbox sucks for several reasons, Exherbo needs a better one.

## Testing ##

- Grab [sys-apps/sydbox](http://git.exherbo.org/summer/packages/sys-apps/sydbox/) from
  [::alip-misc](http://git.exherbo.org/summer/repositories/alip-misc/).
- Switch to sydbox with:

    $ sudo sed -i -e 's/sandbox/sydbox/g' /usr/share/paludis/eapis/exheres-0.conf

- To switch back:

    $ sudo sed -i -e 's/sydbox/sandbox/g' /usr/share/paludis/eapis/exheres-0.conf

- If something goes wrong **PALUDIS\_DO\_NOTHING\_SANDBOXY** is your friend.

## Bugs/Support ##

- Join our channel [#sydbox](irc://irc.freenode.net/sydbox) on [freenode](http://freenode.net/).

## Contribute ##

- Clone [git://github.com/alip/sydbox.git](git://github.com/alip/sydbox.git).
- Read [TODO](http://github.com/alip/sydbox/blob/master/TODO.mkd) and see open [bugs](http://bit.ly/MzeIv).
- Format patches are preferred. Either mail us or poke us on IRC.

<!-- vim: set tw=100 ft=mkd spell spelllang=en sw=4 sts=4 et : -->