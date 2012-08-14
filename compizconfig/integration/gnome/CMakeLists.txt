include (FindPkgConfig)

add_subdirectory (gconf)
add_subdirectory (gsettings)

if (USE_GCONF)

    set (_deps
	glib-2.0>=2.20.0
	gio-2.0>=2.25.0)

    compiz_pkg_check_modules (COMPIZCONFIG_GNOME_INTEGRATION ${_deps})

    if (COMPIZCONFIG_GNOME_INTEGRATION_FOUND)

	add_definitions (-DUSE_GCONF)

	include_directories (${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/gnome/include
			     ${COMPIZCONFIG_GNOME_INTEGRATION_INCLUDE_DIRS})

	link_directories (${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig
			  ${COMPIZCONFIG_GNOME_INTEGRATION_LIBRARY_DIRS})

	add_library (compizconfig_gnome_integration_constants STATIC
		    ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integration_constants.c)

	add_library (compizconfig_gnome_integration STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integration.c)

	target_link_libraries (compizconfig_gnome_integration
			       compizconfig_gnome_integration_constants
			       compizconfig_gnome_integrated_setting
			       compizconfig
			       ${COMPIZCONFIG_GNOME_INTEGRATION_LIBRARIES})

	add_library (compizconfig_gnome_integrated_setting STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integrated_setting.c)

	target_link_libraries (compizconfig_gnome_integrated_setting
			       compizconfig_gnome_integration_constants
			       compizconfig
			       ${COMPIZCONFIG_GNOME_INTEGRATION_LIBRARIES})

    endif (COMPIZCONFIG_GNOME_INTEGRATION_FOUND)

endif (USE_GCONF)
