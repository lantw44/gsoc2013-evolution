module_LTLIBRARIES = module-mailto-handler.la

module_mailto_handler_la_CPPFLAGS = \
	$(AM_CPPFLAGS)						\
	-I$(top_srcdir)						\
	-DG_LOG_DOMAIN=\"evolution-mailto-handler\"		\
	$(EVOLUTION_DATA_SERVER_CFLAGS)				\
	$(GNOME_PLATFORM_CFLAGS)				\
	$(GTKHTML_CFLAGS)

module_mailto_handler_la_SOURCES = \
	evolution-mailto-handler.c

module_mailto_handler_la_LIBADD = \
	$(top_builddir)/e-util/libevolution-util.la		\
	$(top_builddir)/shell/libevolution-shell.la		\
	$(EVOLUTION_DATA_SERVER_LIBS)				\
	$(GNOME_PLATFORM_LIBS)					\
	$(GTKHTML_LIBS)

module_mailto_handler_la_LDFLAGS = \
	-module -avoid-version $(NO_UNDEFINED)

-include $(top_srcdir)/git.mk
