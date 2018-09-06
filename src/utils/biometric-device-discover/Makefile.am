NULL =

SUBDIRS = community-multidevice

discoverdir	= $(prefix)/bin/
dist_discover_SCRIPTS = biometric-device-discover

discover_tools_dir = $(libexecdir)/discover-tools

DOMAIN_NAME = -DLIBBIOMETRIC_DOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

COMM_CFS = -Wall -g -fpic \
	-I$(top_srcdir)/src/lib $(LOCALEDIR_CFS) $(DOMAIN_NAME)
	$(NULL)

COMM_LDFS = -L$(top_srcdir)/src/lib -lbiometric

if HAS_BIO_DRIVER_DIR
COMM_CFS += -DBIO_DRIVER_DIR=$(discover_tools)
endif

biometric-device-discover:
	sed 's|^discover_tools_dir.*$$|discover_tools_dir=$(discover_tools_dir)|g' \
		biometric-device-discover.in > biometric-device-discover
	chmod a+x biometric-device-discover

clean-local:
	rm -rf biometric-device-discover

