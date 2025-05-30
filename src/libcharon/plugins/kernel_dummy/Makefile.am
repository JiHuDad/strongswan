AM_CPPFLAGS = \
	-I${linux_headers} \
	-I$(top_srcdir)/src/libstrongswan \
	-I$(top_srcdir)/src/libcharon

AM_CFLAGS = \
	$(PLUGIN_CFLAGS)

if MONOLITHIC
noinst_LTLIBRARIES = libstrongswan-kernel-dummy.la
else
plugin_LTLIBRARIES = libstrongswan-kernel-dummy.la
endif

libstrongswan_kernel_dummy_la_SOURCES = \
	kernel_dummy_plugin.h kernel_dummy_plugin.c \
	kernel_dummy_ipsec.h kernel_dummy_ipsec.c \
	kernel_dummy_net.h kernel_dummy_net.c

libstrongswan_kernel_dummy_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = kernel_dummy_plugin.h kernel_dummy_plugin.c \
             kernel_dummy_ipsec.h kernel_dummy_ipsec.c \
             kernel_dummy_net.h kernel_dummy_net.c 