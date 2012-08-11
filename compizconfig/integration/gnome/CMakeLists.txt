include (FindPkgConfig)

if (USE_GCONF)

    set (_deps
	glib-2.0>=2.20.0
	gio-2.0>=2.25.0
	gconf-2.0>=2.31.1)

    compiz_pkg_check_modules (COMPIZCONFIG_GNOME_INTEGRATION ${_deps})

    if (COMPIZCONFIG_GNOME_INTEGRATION_FOUND)

	add_definitions (-DUSE_GCONF)

	include_directories (${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/gnome/include
			     ${COMPIZCONFIG_GNOME_INTEGRATION_INCLUDE_DIRS})

	link_directories (${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig
			  ${COMPIZCONFIG_GNOME_INTEGRATION_LIBRARY_DIRS})

	add_library (compizconfig_gnome_integration STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integration.c)

	target_link_libraries (compizconfig_gnome_integration
			       compizconfig
			       ${COMPIZCONFIG_GNOME_INTEGRATION_LIBRARIES})

    endif (COMPIZCONFIG_GNOME_INTEGRATION_FOUND)

endif (USE_GCONF)
