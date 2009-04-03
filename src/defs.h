/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel
 *
 * This file is part of the sydbox sandbox tool. sydbox is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * sydbox is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SYDBOX_GUARD_DEFS_H
#define SYDBOX_GUARD_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif /* HAVE_SYS_REG_H */

/* pink floyd */
#define PINK_FLOYD  "       ..uu.\n" \
                    "       ?$\"\"`?i           z'\n" \
                    "       `M  .@\"          x\"\n" \
                    "       'Z :#\"  .   .    f 8M\n" \
                    "       '&H?`  :$f U8   <  MP   x#'\n" \
                    "       d#`    XM  $5.  $  M' xM\"\n" \
                    "     .!\">     @  'f`$L:M  R.@!`\n" \
                    "    +`  >     R  X  \"NXF  R\"*L\n" \
                    "        k    'f  M   \"$$ :E  5.\n" \
                    "        %%    `~  \"    `  'K  'M\n" \
                    "            .uH          'E   `h\n" \
                    "         .x*`             X     `\n" \
                    "      .uf`                *\n" \
                    "    .@8     .\n" \
                    "   'E9F  uf\"          ,     ,\n" \
                    "     9h+\"   $M    eH. 8b. .8    .....\n" \
                    "    .8`     $'   M 'E  `R;'   d?\"\"\"`\"#\n" \
                    "   ` E      @    b  d   9R    ?*     @\n" \
                    "     >      K.zM `%%M'   9'    Xf   .f\n" \
                    "    ;       R'          9     M  .=`\n" \
                    "    t                   M     Mx~\n" \
                    "    @                  lR    z\"\n" \
                    "    @                  `   ;\"\n" \
                    "                          `\n"

/* environment */
#define ENV_LOG         "SANDBOX_LOG"
#define ENV_CONFIG      "SANDBOX_CONFIG"
#define ENV_WRITE       "SANDBOX_WRITE"
#define ENV_PREDICT     "SANDBOX_PREDICT"
#define ENV_NET         "SANDBOX_NET"
#define ENV_NO_COLOUR   "SANDBOX_NO_COLOUR"

#endif /* SYDBOX_GUARD_DEFS_H */
