MAINTAINERCLEANFILES = Makefile.in

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"
AM_CFLAGS = -Wall @GLIB_CFLAGS@ @DBUS_GLIB_CFLAGS@ @FRAMEWORKD_GLIB_CFLAGS@ @LIBPHONE_UTILS_CFLAGS@

lib_LTLIBRARIES = libphoneui.la

libphoneui_la_SOURCES = phoneui.c phoneui.h phoneui-utility.h phoneui-utility.c 
libphoneui_HEADERS = phoneui.h phoneui-utility.h


libphoneuidir = $(includedir)/phoneui

libphoneui_la_LDFLAGS = $(all_libraries) -ldl

libphoneui_la_LIBADD = @GLIB_LIBS@ @DBUS_GLIB_LIBS@ @FRAMEWORKD_GLIB_LIBS@ @LIBPHONE_UTILS_LIBS@
