CLEANFILES= syscallent.h syscall_marshaller.c syscall_marshaller.h gmon.out

AM_CFLAGS= -DSYSCONFDIR=\"$(sysconfdir)\" -DGIT_HEAD=\"$(GIT_HEAD)\" \
	   $(glib_CFLAGS) $(gobject_CFLAGS) $(confuse_CFLAGS) @SYDBOX_CFLAGS@
bin_PROGRAMS = sydbox
sydbox_SOURCES = children.h context.h log.h loop.h \
		 path.h proc.h syscall.h trace.h util.h wrappers.h \
		 path.c proc.c children.c util.c \
		 context.c syscall.c trace.c wrappers.c loop.c \
		 log.c main.c sydbox-config.c
sydbox_LDADD= $(glib_LIBS) $(gobject_LIBS) $(confuse_LIBS)

sydbox_SOURCES+= syscallent.h
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

sydbox_SOURCES+= syscall_marshaller.h syscall_marshaller.c
BUILT_SOURCES+= syscall_marshaller.h syscall_marshaller.c
syscall_marshaller.h: syscall_marshaller.list
	glib-genmarshal --prefix syscall_marshall --header $< > $@
syscall_marshaller.c: syscall_marshaller.list
	glib-genmarshal --prefix syscall_marshall --body $< > $@

