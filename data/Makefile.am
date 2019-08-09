NULL =

dbusifdir = $(datadir)/dbus-1/interfaces
dist_dbusif_DATA = org.ukui.Biometric.xml

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dist_dbusconf_DATA = org.ukui.Biometric.conf

servicedir = /lib/systemd/system/
dist_service_DATA = biometric-authentication.service

initddir = /etc/init.d/
dist_initd_SCRIPTS = biometric-authentication

polkitdir = $(prefix)/share/polkit-1/actions

dist_polkit_DATA = org.ukui.biometric.policy \
	org.freedesktop.policykit.pkexec.biometric-authentication.policy \
	$(NULL)

if HAS_BIO_CONFIG_DIR
confdir = $(BIO_CONFIG_DIR)
else
confdir = $(sysconfdir)/biometric-auth
endif

driversconfdir = $(confdir)
dist_driversconf_DATA = biometric-drivers.conf
