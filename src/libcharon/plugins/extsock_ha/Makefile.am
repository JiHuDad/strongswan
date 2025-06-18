AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libstrongswan \
	-I$(top_srcdir)/src/libcharon

AM_CFLAGS = \
	$(PLUGIN_CFLAGS)

if MONOLITHIC
noinst_LTLIBRARIES = libstrongswan-extsock-ha.la
else
plugin_LTLIBRARIES = libstrongswan-extsock-ha.la
endif

libstrongswan_extsock_ha_la_SOURCES = \
	extsock_ha_plugin.h extsock_ha_plugin.c

libstrongswan_extsock_ha_la_LDFLAGS = -module -avoid-version
libstrongswan_extsock_ha_la_LIBADD = $(CJSON_LIBS) 