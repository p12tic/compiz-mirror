project (compizconfig-backend-gsettings)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)
include (CompizGSettings)

compizconfig_backend (gsettings PKGDEPS glib-2.0>=2.20.0 gconf-2.0>=2.31.1 gio-2.0>=2.25.0)

compiz_install_gsettings_schema (${CMAKE_SOURCE_DIR}/org.freedesktop.compizconfig.gschema.xml
				 ${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)

compiz_print_configure_header ("CompizConfig GSettings Storage Backend")
compiz_print_configure_footer ()
compiz_package_generation ("CompizConfig GSettings Storage Backend")
