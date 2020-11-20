NULL =

AUTOMAKE_OPTIONS = subdir-objects

if HAS_BIO_DRIVER_DIR
drv_dir = $(BIO_DRIVER_DIR)
else
drv_dir = $(datadir)/biometric-authentication/driver
endif

if HAS_BIO_EXTRA_DIR
extra_dir = $(BIO_EXTRA_DIR)
else
extra_dir = $(datadir)/biometric-authentication/driver/extra
endif

DOMAIN_NAME = -DBIOMETRIC_DRIVER_COMMUNITY_MULTIDEVICE_DOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

libdrvdir = $(drv_dir)
libdrv_LTLIBRARIES = upekts.la uru4000.la aes4000.la aes2501.la upektc.la \
	aes1610.la fdu2000.la vcom5s.la upeksonly.la vfs101.la vfs301.la \
	aes2550.la aes1660.la aes2660.la aes3500.la upektc_img.la \
	etes603.la vfs5011.la vfs0050.la elan.la \
	$(NULL)

COMM_SOURCES = $(top_srcdir)/src/drivers/community-multidevice/community_ops.c \
	$(top_srcdir)/src/drivers/community-multidevice/aes_128_cfb.c \
	$(top_srcdir)/src/drivers/community-multidevice/close.c \
	$(top_srcdir)/src/drivers/community-multidevice/driver_internal.c

COMM_INCLUDE = $(top_srcdir)/src/drivers/community-multidevice/close.h \
	$(top_srcdir)/src/drivers/community-multidevice/community_define.h \
	$(top_srcdir)/src/drivers/community-multidevice/aes_128_cfb.h \
	$(top_srcdir)/src/drivers/community_multidevice/driver_internal.h \
	$(top_srcdir)/src/drivers/comunity_multidevice/driver_type.h \
	$(NULL)

COMM_CFS = -Wall -g -o0 -shared -fpic \
	-I$(top_srcdir)/src/lib/ -I$(top_srcdir)/src/drivers/community-multidevice/ \
	$(LOCALEDIR_CFS) $(DOMAIN_NAME) \
	$(LIBBIOMETRIC_CFLAGS) $(LIBFPRINT_CFLAGS) \
	$(GLIB_2_CFLAGS) $(LIBUSB_1_CFLAGS) $(SQLITE3_CFLAGS) \
	$(NULL)

COMM_LDFS = $(LIBFPRINT_LIBS) $(GLIB_2_LIBS) $(LIBUSB_1_LIBS) $(SQLITE3_LIBS) \
	../../../lib/libbiometric.la -module -avoid-version -Wl,-z,now \
	$(NULL)

upekts_la_SOURCES = $(COMM_SOURCES) upekts.c
upekts_la_INCLUDE = $(COMM_INCLUDE)
upekts_la_CFLAGS = $(COMM_CFS)
upekts_la_LDFLAGS = $(COMM_LDFS)

uru4000_la_SOURCES = $(COMM_SOURCES) uru4000.c
uru4000_la_INCLUDE = $(COMM_INCLUDE)
uru4000_la_CFLAGS = $(COMM_CFS)
uru4000_la_LDFLAGS = $(COMM_LDFS)

aes4000_la_SOURCES = $(COMM_SOURCES) aes4000.c
aes4000_la_INCLUDE = $(COMM_INCLUDE)
aes4000_la_CFLAGS = $(COMM_CFS)
aes4000_la_LDFLAGS = $(COMM_LDFS)

aes2501_la_SOURCES = $(COMM_SOURCES) aes2501.c
aes2501_la_INCLUDE = $(COMM_INCLUDE)
aes2501_la_CFLAGS = $(COMM_CFS)
aes2501_la_LDFLAGS = $(COMM_LDFS)

upektc_la_SOURCES = $(COMM_SOURCES) upektc.c
upektc_la_INCLUDE = $(COMM_INCLUDE)
upektc_la_CFLAGS = $(COMM_CFS)
upektc_la_LDFLAGS = $(COMM_LDFS)

aes1610_la_SOURCES = $(COMM_SOURCES) aes1610.c
aes1610_la_INCLUDE = $(COMM_INCLUDE)
aes1610_la_CFLAGS = $(COMM_CFS)
aes1610_la_LDFLAGS = $(COMM_LDFS)

fdu2000_la_SOURCES = $(COMM_SOURCES) fdu2000.c
fdu2000_la_INCLUDE = $(COMM_INCLUDE)
fdu2000_la_CFLAGS = $(COMM_CFS)
fdu2000_la_LDFLAGS = $(COMM_LDFS)

vcom5s_la_SOURCES = $(COMM_SOURCES) vcom5s.c
vcom5s_la_INCLUDE = $(COMM_INCLUDE)
vcom5s_la_CFLAGS = $(COMM_CFS)
vcom5s_la_LDFLAGS = $(COMM_LDFS)

upeksonly_la_SOURCES = $(COMM_SOURCES) upeksonly.c
upeksonly_la_INCLUDE = $(COMM_INCLUDE)
upeksonly_la_CFLAGS = $(COMM_CFS)
upeksonly_la_LDFLAGS = $(COMM_LDFS)

vfs101_la_SOURCES = $(COMM_SOURCES) vfs101.c
vfs101_la_INCLUDE = $(COMM_INCLUDE)
vfs101_la_CFLAGS = $(COMM_CFS)
vfs101_la_LDFLAGS = $(COMM_LDFS)

vfs301_la_SOURCES = $(COMM_SOURCES) vfs301.c
vfs301_la_INCLUDE = $(COMM_INCLUDE)
vfs301_la_CFLAGS = $(COMM_CFS)
vfs301_la_LDFLAGS = $(COMM_LDFS)

aes2550_la_SOURCES = $(COMM_SOURCES) aes2550.c
aes2550_la_INCLUDE = $(COMM_INCLUDE)
aes2550_la_CFLAGS = $(COMM_CFS)
aes2550_la_LDFLAGS = $(COMM_LDFS)

aes1660_la_SOURCES = $(COMM_SOURCES) aes1660.c
aes1660_la_INCLUDE = $(COMM_INCLUDE)
aes1660_la_CFLAGS = $(COMM_CFS)
aes1660_la_LDFLAGS = $(COMM_LDFS)

aes2660_la_SOURCES = $(COMM_SOURCES) aes2660.c
aes2660_la_INCLUDE = $(COMM_INCLUDE)
aes2660_la_CFLAGS = $(COMM_CFS)
aes2660_la_LDFLAGS = $(COMM_LDFS)

aes3500_la_SOURCES = $(COMM_SOURCES) aes3500.c
aes3500_la_INCLUDE = $(COMM_INCLUDE)
aes3500_la_CFLAGS = $(COMM_CFS)
aes3500_la_LDFLAGS = $(COMM_LDFS)

upektc_img_la_SOURCES = $(COMM_SOURCES) upektc_img.c
upektc_img_la_INCLUDE = $(COMM_INCLUDE)
upektc_img_la_CFLAGS = $(COMM_CFS)
upektc_img_la_LDFLAGS = $(COMM_LDFS)

etes603_la_SOURCES = $(COMM_SOURCES) etes603.c
etes603_la_INCLUDE = $(COMM_INCLUDE)
etes603_la_CFLAGS = $(COMM_CFS)
etes603_la_LDFLAGS = $(COMM_LDFS)

vfs5011_la_SOURCES = $(COMM_SOURCES) vfs5011.c
vfs5011_la_INCLUDE = $(COMM_INCLUDE)
vfs5011_la_CFLAGS = $(COMM_CFS)
vfs5011_la_LDFLAGS = $(COMM_LDFS)

vfs0050_la_SOURCES = $(COMM_SOURCES) vfs0050.c
vfs0050_la_INCLUDE = $(COMM_INCLUDE)
vfs0050_la_CFLAGS = $(COMM_CFS)
vfs0050_la_LDFLAGS = $(COMM_LDFS)

elan_la_SOURCES = $(COMM_SOURCES) elan.c
elan_la_INCLUDE = $(COMM_INCLUDE)
elan_la_CFLAGS = $(COMM_CFS)
elan_la_LDFLAGS = $(COMM_LDFS)


