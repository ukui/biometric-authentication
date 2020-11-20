NULL =

AUTOMAKE_OPTIONS = subdir-objects

if HAS_BIO_DRIVER_DIR
drv_dir = $(BIO_DRIVER_DIR)
else
drv_dir = $(datadir)/biometric-authentication/driver/
endif

discover_tools_dir = $(drv_dir)/../discover-tools/extra/

discover_toolsdir = $(discover_tools_dir)
discover_tools_PROGRAMS = community-multidevice-discover-tool

DOMAIN_NAME = -DBIOMETRIC_DRIVER_COMMUNITY_MULTIDEVICE_DOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

COMM_CFS = -Wall -g -fpic \
	-I$(top_srcdir)/src/ -I$(top_srcdir)/src/drivers/community-multidevice/ \
	$(LOCALEDIR_CFS) $(DOMAIN_NAME)
	$(NULL)

COMM_LIBS = -L$(top_srcdir)/src/
COMM_LDFS = -Wl,-z,now

community_multidevice_discover_tool_SOURCES = \
	community_multidevice_discover_tool.c \
	$(NULL)

community_multidevice_discover_tool_INCLUDE = \
	community_multidevice_discover_tool.h \
	community_define.h \
	$(NULL)

community_multidevice_discover_tool_CFLAGS = $(COMM_CFS) \
	$(LIBFPRINT_CFLAGS) $(GLIB_2_CFLAGS) $(LIBUSB_1_CFLAGS) $(SQLITE3_CFLAGS) \
	$(NULL)

community_multidevice_discover_tool_LDADD = $(COMM_LIBS) \
	$(LIBFPRINT_LIBS) $(GLIB_2_LIBS) $(LIBUSB_1_LIBS) $(SQLITE3_LIBS) \
	$(NULL)

community_multidevice_discover_tool_LDFLAGS = $(COMM_LDFS)

install-exec-local:
	mkdir -p $(DESTDIR)/$(drv_dir)/../discover-tools/
	DRIVER_LIST="upekts uru4000 aes4000 aes2501 upektc aes1610 fdu2000 vcom5s \
		upeksonly vfs101 vfs301 aes2550 aes1660 aes2660 aes3500  \
		upektc_img etes603 vfs5011 vfs0050 elan"; \
	for driver in $${DRIVER_LIST}; do \
		ln -sf extra/community-multidevice-discover-tool \
			$(DESTDIR)/$(drv_dir)/../discover-tools/usb-$${driver}; \
	done


