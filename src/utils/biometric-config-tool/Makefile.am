NULL =

bin_PROGRAMS = biometric-config-tool

DOMAIN_NAME_CFS = -DLIBBIOMETRIC_DOMAIN_NAME=\""$(PACKAGE)"\"
LOCALEDIR_CFS = -DLOCALEDIR=\""$(localedir)"\"

COMM_CFS = -Wall -g $(LOCALEDIR_CFS) $(DOMAIN_NAME_CFS) -I$(top_srcdir)/src/lib
COMM_LDFS = -Wl,-z,now

biometric_config_tool_SOURCES = biometric-config-tool-main.c biometric-config-tool-main.h \
	biometric-config-tool-add-driver.c biometric-config-tool-add-driver.h \
	biometric-config-tool-remove-driver.c biometric-config-tool-remove-driver.h \
	biometric-config-tool-enable-driver.c biometric-config-tool-enable-driver.h \
	biometric-config-tool-disable-driver.c biometric-config-tool-disable-driver.h \
	biometric-config-tool-set-key.c biometric-config-tool-set-key.h \
	biometric-config-tool-remove-key.c biometric-config-tool-remove-key.h \
	$(NULL)

biometric_config_tool_INCLUDE = biometric_config.h

biometric_config_tool_CFLAGS = $(COMM_CFS) \
	$(GLIB_2_CFLAGS) $(GIO_2_CFLAGS) $(GIO_UNIX_2_CFLAGS) \
	$(NULL)

biometric_config_tool_LDADD = $(GLIB_2_LIBS) $(GIO_2_LIBS) $(GIO_UNIX_2_LIBS) \
	$(NULL)

biometric_config_tool_LDFLAGS = $(COMM_LDFS)
