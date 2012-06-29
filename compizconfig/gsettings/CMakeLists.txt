project (compizconfig-backend-gsettings)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)
include (CompizGSettings)

set (_deps glib-2.0>=2.20.0
	  gio-2.0>=2.25.0)

option (USE_GCONF "Enable legacy GNOME 2.x option integration with GConf" ON)

if (USE_GCONF)
    list (APPEND _deps gconf-2.0>=2.31.1)

    add_definitions (-DUSE_GCONF)
endif (USE_GCONF)

compizconfig_backend (gsettings PKGDEPS ${_deps})

compiz_install_gsettings_schema (${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.compizconfig.gschema.xml
				 ${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)

add_subdirectory (tests)
