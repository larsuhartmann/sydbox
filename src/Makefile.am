CLEANFILES= syscallent.h

AM_CFLAGS= -DSYSCONFDIR=\"$(sysconfdir)\" -DGITHEAD=\"$(GITHEAD)\" $(confuse_CFLAGS) @SYDBOX_CFLAGS@
bin_PROGRAMS = sydbox
sydbox_SOURCES = defs.h syscallent.h \
		 children.c util.c context.c path.c \
		 realpath.c syscall.c trace.c main.c
sydbox_LDADD= $(confuse_LIBS)

syscallent.h:
	@echo "Generating $@ from asm/unistd.h"
	echo "// This is a GENERATED file!" > $@
	echo '' | $(CC) -E --include asm/unistd.h -dM - |\
		sort -n -k 3 |\
		while read line; do \
			if [ "$${line#\#define __NR_}" != "$$line" ]; then \
				no="$$(echo \"$$line\" | cut -d' ' -f2)"; \
				name="$$(echo \"$$no\" | $(SED) -e 's:__NR_::')"; \
				echo "{$$no, $$name}," >> $@ ;\
			fi \
		done

