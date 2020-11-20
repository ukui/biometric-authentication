#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([biometric-authentication], [0.9.62], [https://github.com/ukui/biometric-authentication/issues])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/biometric-authenticationd.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])
IT_PROG_INTLTOOL([0.40])

# Checks for libraries.

PKG_CHECK_MODULES(GLIB_2, glib-2.0 >= 2.48)
PKG_CHECK_MODULES(GIO_2, gio-2.0 >= 2.48)
PKG_CHECK_MODULES(GIO_UNIX_2, gio-unix-2.0 >= 2.48)
PKG_CHECK_MODULES(GOBJECT_2, gobject-2.0 >= 2.48)
PKG_CHECK_MODULES(GTK_3, gtk+-3.0 >= 3.18)
PKG_CHECK_MODULES(LIBFPRINT, libfprint-2 >= 1.90.1)
PKG_CHECK_MODULES(LIBUSB_1, libusb-1.0 >= 1.0.20)
PKG_CHECK_MODULES(SQLITE3, sqlite3 >= 3.8)
PKG_CHECK_MODULES(UUID, uuid >= 2.27)
PKG_CHECK_MODULES(POLKIT, polkit-gobject-1 >= 0.105)
AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([pthread], [pthread_mutex_trylock])
AC_CHECK_LIB([sqlite3], [sqlite3_prepare_v2])
AC_CHECK_LIB([libfprint], [fp_discover_devs])
AC_CHECK_LIB([usb-1.0], [libusb_hotplug_register_callback])
AC_CHECK_LIB([glib-2.0], [g_key_file_get_string])
AC_CHECK_LIB([uuid], [uuid_generate])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h memory.h stdint.h stdlib.h \
				  string.h termios.h unistd.h dlfcn.h libgen.h \
				  libfprint/fprint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero memset mkdir pow strerror dlopen dlclose dlsym dlerror \
				pthread_create pthread_mutex_init pthread_mutex_trylock pthread_mutex_unlock \
				sqlite3_bind_int64 sqlite3_bind_parameter_index sqlite3_bind_text \
				sqlite3_column_int sqlite3_column_text sqlite3_close_v2 sqlite3_errmsg \
				sqlite3_exec sqlite3_finalize sqlite3_open_v2 sqlite3_close_v2 \
				sqlite3_prepare_v2 sqlite3_reset sqlite3_step \
				libusb_error_name libusb_exit libusb_get_device_descriptor \
				libusb_hotplug_register_callback libusb_init \
				g_error_free g_key_file_free g_key_file_get_boolean g_key_file_get_groups \
				g_key_file_get_string g_key_file_load_from_file g_key_file_new \
				g_list_append g_list_foreach g_list_free g_list_index g_list_remove \
				uuid_generate uuid_unparse_lower uuid_parse])

GETTEXT_PACKAGE=${PACKAGE_TARNAME}
AC_SUBST([GETTEXT_PACKAGE])

AC_CANONICAL_HOST

case "$host_cpu" in
	aarch64*)
		HOST_ARCH=arm64
		;;
	x86?64*)
		HOST_ARCH=amd64
		;;
esac

AM_CONDITIONAL(NEED_UD650, [test "X$HOST_ARCH" = "Xamd64" -o "X$HOST_ARCH" = "Xarm64"])


AC_ARG_WITH([bio-db-dir],
		AS_HELP_STRING([--with-bio-db-dir=DIR], [Directory for database file]),
		[BIO_DB_DIR="$withval"],
		[BIO_DB_DIR=""])
AC_SUBST([BIO_DB_DIR])
AM_CONDITIONAL(HAS_BIO_DB_DIR, test "X$BIO_DB_DIR" != "X")

AC_ARG_WITH([bio-db-name],
		AS_HELP_STRING([--with-bio-db-name=FILE], [Database file name]),
		[BIO_DB_NAME="$withval"],
		[BIO_DB_NAME=""])
AC_SUBST([BIO_DB_NAME])
AM_CONDITIONAL(HAS_BIO_DB_NAME, test "X$BIO_DB_NAME" != "X")

AC_ARG_WITH([bio-config-dir],
		AS_HELP_STRING([--with-bio-config-dir=DIR], [Directory for biometric config file]),
		[BIO_CONFIG_DIR="$withval"],
		[BIO_CONFIG_DIR="$"])
AC_SUBST([BIO_CONFIG_DIR])
AM_CONDITIONAL(HAS_BIO_CONFIG_DIR, test "X$BIO_CONFIG_DIR" != "X")

AC_ARG_WITH([bio-driver-dir],
		AS_HELP_STRING([--with-bio-driver-dir=DIR], [Directory for driver files]),
		[BIO_DRIVER_DIR="$withval"],
		[BIO_DRIVER_DIR=""])
AC_SUBST([BIO_DRIVER_DIR])
AM_CONDITIONAL(HAS_BIO_DRIVER_DIR, test "X$BIO_DRIVER_DIR" != "X")

AC_ARG_WITH([bio-extra-dir],
		AS_HELP_STRING([--with-bio-extra-dir=DIR], [Directory for extra files(driver and extra tools)]),
		[BIO_EXTRA_DIR="$withval"],
		[BIO_EXTRA_DIR=""])
AC_SUBST([BIO_EXTRA_DIR])
AM_CONDITIONAL(HAS_BIO_EXTRA_DIR, test "X$BIO_EXTRA_DIR" != "X")

AC_CONFIG_FILES([Makefile
				data/Makefile
				src/Makefile
				src/lib/Makefile
				src/lib/libbiometric.pc
				src/drivers/Makefile
				src/drivers/community-multidevice/Makefile
				src/drivers/community-multidevice/drivers/Makefile
				src/utils/Makefile
				src/utils/biometric-auth-client/Makefile
				src/utils/biometric-config-tool/Makefile
				src/utils/biometric-device-discover/Makefile
				src/utils/biometric-device-discover/community-multidevice/Makefile
				po/Makefile.in])
AC_OUTPUT
