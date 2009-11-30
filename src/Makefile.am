MAINTAINERCLEANFILES = Makefile.in

localedir = $(datadir)/locale
AM_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" -DG_LOG_DOMAIN=\"libphone-ui\"
AM_CFLAGS = -Wall -Wextra -Werror @GLIB_CFLAGS@ @DBUS_GLIB_CFLAGS@ @FRAMEWORKD_GLIB_CFLAGS@ @LIBPHONE_UTILS_CFLAGS@ @ALSA_CFLAGS@

lib_LTLIBRARIES = libphone-ui.la

libphone_ui_la_SOURCES = phoneui.c phoneui.h phoneui-utils.h phoneui-utils.c \
			 phoneui-utils-sound.c phoneui-utils-sound.h \
			 phoneui-utils-device.c phoneui-utils-device.h \
			 phoneui-utils-feedback.c phoneui-utils-feedback.h \
libphone_ui_HEADERS = phoneui.h phoneui-utils.h phoneui-utils-sound.h \
		      phoneui-utils-device.h phoneui-utils-feedback.h


libphone_uidir = $(includedir)/phoneui

libphone_ui_la_LDFLAGS = $(all_libraries) -ldl

libphone_ui_la_LIBADD = @GLIB_LIBS@ @DBUS_GLIB_LIBS@ @FRAMEWORKD_GLIB_LIBS@ @LIBPHONE_UTILS_LIBS@ @ALSA_LIBS@
