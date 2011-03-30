find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (notification PKGDEPS libnotify gtk+-2.0)
compiz_pkg_check_modules (HAVE_LIBNOTIFY_0_6 libnotify>=0.6)
if (HAVE_LIBNOTIFY_0_6_FOUND)
	add_definitions (-DHAVE_LIBNOTIFY_0_6)
endif (HAVE_LIBNOTIFY_0_6_FOUND)
