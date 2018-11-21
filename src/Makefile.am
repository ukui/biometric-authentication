NULL =

SUBDIRS = lib drivers utils

DESC_XML = $(top_srcdir)/data/org.ukui.Biometric.xml
GENERATED_CH = biometric-generated

libexec_PROGRAMS = biometric-authenticationd

COMM_CFS = -Wall -g -I$(top_srcdir)/src/lib
COMM_LIBS = -Llib -lbiometric
COMM_LDFS = -Wl,-z,now

DOMAIN_NAME = -DDOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

biometric_authenticationd_SOURCES = \
	biometric-authenticationd.h biometric-authenticationd.c \
	biometric-dbus-common.h biometric-dbus-common.c \
	biometric-generated.h biometric-generated.c \
	dbus_comm.h \
	$(NULL)

biometric_authenticationd_INCLUDE = biometric_common.h biometric_version.h

biometric_authenticationd_CFLAGS = $(COMM_CFS) $(LOCALEDIR_CFS) \
	$(GLIB_2_CFLAGS) $(GIO_2_CFLAGS) $(GIO_UNIX_2_CFLAGS) $(GTK_3_CFLAGS) \
	$(GOBJECT_2_CFLAGS) $(LIBUSB_1_CFLAGS) $(SQLITE3_CFLAGS) $(POLKIT_CFLAGS) \
	$(NULL)

biometric_authenticationd_LDADD = $(COMM_LIBS) \
	$(GLIB_2_LIBS) $(GIO_2_LIBS) $(GIO_UNIX_2_LIBS) $(GTK_3_LIBS) \
	$(GOBJECT_2_LIBS) $(LIBUSB_1_LIBS) $(SQLITE3_LIBS) $(POLKIT_LIBS) \
	$(NULL)

biometric_authenticationd_LDFLAGS = $(COMM_LDFS)

biometric-authenticationd.h: biometric-generated.h

biometric-authenticationd.c: biometric-generated.c

biometric-generated.h biometric-generated.c: $(DESC_XML)
	gdbus-codegen --interface-prefix org.ukui. --generate-c-code $(GENERATED_CH) $(DESC_XML)

clean-local:
	rm -rf biometric-generated.h biometric-generated.c
