CLEANFILES= *.log arnold_layne its_not_the_same

TESTS= check_sydbox t01-chmod.bash t02-chown.bash t03-open.bash t04-creat.bash \
       t05-lchown.bash t06-link.bash t07-mkdir.bash t08-mknod.bash t09-access.bash \
       t10-rename.bash
check_PROGRAMS= check_sydbox test-lib.bash t01_chmod t02_chown t03_open \
		t04_creat t05_lchown t06_link t07_mkdir t08_mknod t09_access \
		t10_rename

check_sydbox_SOURCES= check_children.c check_path.c check_trace.c check_syscall.c \
		      check_sydbox.h check_sydbox.c \
		      $(top_builddir)/src/defs.h $(top_builddir)/src/children.c \
		      $(top_builddir)/src/util.c $(top_builddir)/src/context.c \
		      $(top_builddir)/src/path.c $(top_builddir)/src/realpath.c \
		      $(top_builddir)/src/syscall.c $(top_builddir)/src/trace.c
check_sydbox_CFLAGS= @SYDBOX_CFLAGS@ @CHECK_CFLAGS@
check_sydbox_LDADD= @CHECK_LIBS@

t01_chmod_SOURCES= t01-chmod.c
t02_chown_SOURCES= t02-chown.c
t03_open_SOURCES= t03-open.c
t04_creat_SOURCES= t04-creat.c
t05_lchown_SOURCES= t05-lchown.c
t06_link_SOURCES= t06-link.c
t07_mkdir_SOURCES= t07-mkdir.c
t08_mknod_SOURCES= t08-mknod.c
t09_access_SOURCES= t09-access.c
t10_rename_SOURCES= t10-rename.c

test-lib.bash: test-lib.bash.in
	sed -e "s/@TOP_BUILDDIR@/$(top_builddir)/" test-lib.bash.in > test-lib.bash
