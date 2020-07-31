NULL =

LIB_SO_VERSION_MAJOR = 1
LIB_SO_VERSION_MINOR = 3
LIB_SONAME = libbiometric.so.$(LIB_SO_VERSION_MAJOR).$(LIB_SO_VERSION_MINOR)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbiometric.pc
DISTCLEANFILES = libbiometric.pc

libbiodir = $(libdir)
libbio_LTLIBRARIES = libbiometric.la

dist-hook:
	echo dist-hook

DOMAIN_NAME = -DLIBBIOMETRIC_DOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

COMM_CFS = -Wall -g $(LOCALEDIR_CFS) $(DOMAIN_NAME)
COMM_LDFS = -Wl,-z,now

if HAS_BIO_DB_DIR
COMM_CFS += -DBIO_DB_DIR=$(BIO_DB_DIR)
endif

if HAS_BIO_DB_NAME
COMM_CFS += -DBIO_DB_NAME=$(BIO_DB_NAME)
endif

if HAS_BIO_CONFIG_DIR
COMM_CFS += -DBIO_CONFIG_DIR=$(BIO_CONFIG_DIR)
endif

libbiometric_la_SOURCES = \
	biometric_common.h biometric_common.c \
	biometric_config.h biometric_config.c \
	biometric_storage.h biometric_storage.c \
	biometric_version.h biometric_version.c \
	$(NULL)

libbiometric_la_CFLAGS = $(COMM_CFS) $(UUID_CFLAGS) \
	$(GLIB_2_CFLAGS) $(GTK_3_CFLAGS) $(LIBUSB_1_CFLAGS) $(SQLITE3_CFLAGS) \
	$(NULL)

libbiometric_la_LDFLAGS = $(COMM_LDFS) $(UUID_LDFLAGS) \
	$(GLIB_2_LIBS) $(GTK_3_LIBS) $(LIBUSB_1_LIBS) $(SQLITE3_LIBS) \
	$(NULL)

libbiometricincludedir=$(includedir)/libbiometric/

libbiometricinclude_HEADERS = biometric_common.h \
	biometric_config.h \
	biometric_storage.h \
	biometric_version.h \
	$(NULL)
