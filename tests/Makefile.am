SUBDIRS = . progtests

TESTS= check_sydbox t01-util-shell-expand.bash
check_PROGRAMS= check_sydbox t01_util_shell_expand

check_sydbox_SOURCES= check_children.c check_path.c check_trace.c check_syscall.c \
		      check_util.c check_sydbox.h check_sydbox.c \
		      $(top_builddir)/src/defs.h $(top_builddir)/src/children.c \
		      $(top_builddir)/src/util.c $(top_builddir)/src/context.c \
		      $(top_builddir)/src/path.c $(top_builddir)/src/syscall.c \
		      $(top_builddir)/src/trace.c $(top_builddir)/src/wrappers.c \
		      $(top_builddir)/src/proc.c
check_sydbox_CFLAGS= @SYDBOX_CFLAGS@ @CHECK_CFLAGS@
check_sydbox_LDADD= @CHECK_LIBS@

t01_util_shell_expand_SOURCES= t01_util_shell_expand.c \
			      $(top_builddir)/src/path.c $(top_builddir)/src/children.c \
			      $(top_builddir)/src/context.c $(top_builddir)/src/wrappers.c \
			      $(top_builddir)/src/util.c
