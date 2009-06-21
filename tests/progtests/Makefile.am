CLEANFILES = *.log test-lib.bash

TESTS = t01-chmod.bash t02-chown.bash t03-open.bash t04-creat.bash t05-lchown.bash \
	t06-link.bash t07-mkdir.bash t08-mknod.bash t09-access.bash \
	t10-rename-first.bash t11-rename-second.bash t12-rmdir.bash t13-symlink.bash \
	t14-truncate.bash t15-mount.bash t16-umount.bash t17-umount2.bash t18-utime.bash \
	t19-unlink.bash t20-renameat-first-atfdcwd.bash t21-renameat-first.bash \
	t22-renameat-second-atfdcwd.bash t23-renameat-second.bash t24-linkat-first-atfdcwd.bash \
	t25-linkat-first.bash t26-linkat-second-atfdcwd.bash t27-linkat-second.bash \
	t28-symlinkat-atfdcwd.bash t29-symlinkat.bash t30-fchmodat-atfdcwd.bash t31-fchmodat.bash \
	t32-magic-onoff.bash t33-magic-enabled.bash t34-magic-lock.bash
EXTRA_DIST= $(TESTS)

check_PROGRAMS = test-lib.bash t01_chmod t01_chmod_toolong t02_chown t02_chown_toolong \
		 t03_open t03_open_toolong t04_creat t04_creat_toolong t05_lchown \
		 t06_link t07_mkdir t07_mkdir_toolong t08_mknod t08_mknod_toolong t09_access \
		 t10_rename_first t11_rename_second t12_rmdir t12_rmdir_toolong t13_symlink \
		 t13_symlink_toolong t14_truncate t14_truncate_toolong t15_mount t16_umount \
		 t17_umount2 t18_utime t19_unlink t19_unlink_toolong t20_renameat_first_atfdcwd \
		 t21_renameat_first t22_renameat_second_atfdcwd t23_renameat_second \
		 t24_linkat_first_atfdcwd t25_linkat_first t26_linkat_second_atfdcwd t27_linkat_second \
		 t28_symlinkat_atfdcwd t29_symlinkat t30_fchmodat_atfdcwd t31_fchmodat \
		 t32_magic_onoff_set_on t32_magic_onoff_set_off t32_magic_onoff_check_off \
		 t32_magic_onoff_check_on

test_lib_bash_SOURCES= test-lib.bash.in

test-lib.bash: test-lib.bash.in
	$(SED) -e "s:@TOP_BUILDDIR@:$(top_builddir):" $< > $@

