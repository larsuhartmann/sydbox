SUBDIRS = .

CLEANFILES= *.log

TESTS= t01-chmod.bash t02-chown.bash t03-open.bash t04-creat.bash t05-lchown.bash \
       t06-link.bash t07-mkdir.bash t08-mknod.bash t09-access.bash \
       t10-rename-first.bash t11-rename-second.bash t12-rmdir.bash t13-symlink.bash \
       t14-truncate.bash t15-mount.bash t16-umount.bash t17-umount2.bash
check_PROGRAMS= test-lib.bash t01_chmod t02_chown t03_open \
		t04_creat t05_lchown t06_link t07_mkdir t08_mknod t09_access \
		t10_rename_first t11_rename_second t12_rmdir t13_symlink t14_truncate \
		t15_mount t16_umount t17_umount2

test-lib.bash: test-lib.bash.in
	sed -e "s:@TOP_BUILDDIR@:$(top_builddir):" test-lib.bash.in > test-lib.bash