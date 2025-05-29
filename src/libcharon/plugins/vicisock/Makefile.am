AM_CPPFLAGS = \
    -I$(top_srcdir)/src/libstrongswan \
    -I$(top_srcdir)/src/libcharon \
    -I$(top_srcdir)/src/libcharon/plugins \
    -I$(top_srcdir)/src/libvici \
    -I/usr/include/cjson \
    $(VICISOCK_CFLAGS)

AM_CFLAGS = \
    $(PLUGIN_CFLAGS)

if MONOLITHIC
noinst_LTLIBRARIES = libstrongswan-vicisock.la
else
plugin_LTLIBRARIES = libstrongswan-vicisock.la
endif

noinst_LTLIBRARIES ?=
plugin_LTLIBRARIES ?=

libstrongswan_vicisock_la_SOURCES = vicisock_plugin.c
libstrongswan_vicisock_la_LIBADD = -L$(top_builddir)/src/libcharon/plugins/vici -lvici $(VICISOCK_LIBS)
libstrongswan_vicisock_la_LDFLAGS = -module -avoid-version -no-undefined
