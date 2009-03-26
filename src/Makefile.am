CLEANFILES= syscallent.h gmon.out

AM_CFLAGS= -DSYSCONFDIR=\"$(sysconfdir)\" -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(confuse_CFLAGS) @SYDBOX_CFLAGS@
bin_PROGRAMS = sydbox
sydbox_SOURCES = defs.h syscallent.h \
		 getcwd.c path.c proc.c children.c util.c \
		 context.c syscall.c trace.c wrappers.c loop.c \
		 main.c
sydbox_LDADD= $(glib_LIBS) $(confuse_LIBS)

BUILT_SOURCES= syscallent.h
if GCC
syscallent.h:
	@echo "Generating $@ from asm/unistd.h"
	echo "// This is a GENERATED file!" > $@
	echo '' | $(CC) $(CFLAGS) -E --include asm/unistd.h -dM - |\
		sort -n -k 3 |\
		while read line; do \
			if [ "$${line#\#define __NR_}" != "$$line" ]; then \
				no="$$(echo \"$$line\" | cut -d' ' -f2)"; \
				name="$$(echo \"$$no\" | $(SED) -e 's:__NR_::')"; \
				echo "{$$no, $$name}," >> $@ ;\
			fi \
		done
else
syscallent.h:
	@echo "Generating dummy syscallent.h because compiler isn't gcc"
	echo >syscallent.h
endif
