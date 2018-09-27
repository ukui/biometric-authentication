NULL =

clientdir	= $(prefix)/bin/
dist_client_SCRIPTS = biometric-auth-client

DOMAIN_NAME = $(PACKAGE)
LOCALEDIR = $(localedir)

biometric-auth-client:
	cp biometric-auth-client.in biometric-auth-client
	sed -i 's#BIO_DOMAIN_NAME#$(DOMAIN_NAME)#g' biometric-auth-client
	sed -i 's#BIO_LOCALEDIR#$(LOCALEDIR)#g' biometric-auth-client
	chmod a+x biometric-auth-client

clean-local:
	rm -rf biometric-auth-client

