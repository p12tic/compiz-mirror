include (FindPkgConfig)

if (USE_GCONF)

    set (_deps
	glib-2.0>=2.20.0
	gio-2.0>=2.25.0)

    compiz_pkg_check_modules (COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION ${_deps})

    if (COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION)

	add_definitions (-DUSE_GCONF)

	include_directories (${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/gsettings/gsettings_backend_shared
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/gnome/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/gnome/gsettings/include
			     ${COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION_INCLUDE_DIRS})

	link_directories (${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig
			  ${CMAKE_BINARY_DIR}/compizconfig/gsettings/gsettings_backend_shared
			  ${CMAKE_BINARY_DIR}/compizconfig/integration/gnome
			  ${COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION_LIBRARY_DIRS})

	add_library (compizconfig_gnome_integration_gsettings_integrated_setting STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integration_gsettings_integrated_setting.c)

	target_link_libraries (compizconfig_gnome_integration_gsettings_integrated_setting
			       compizconfig_gnome_integrated_setting
			       compizconfig_gnome_integration_constants
			       compizconfig
			       gsettings_backend_shared
			       ${COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION_LIBRARIES})

	add_library (compizconfig_gnome_integration_gsettings_setting_factory STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_gnome_integration_gsettings_integrated_setting_factory.c)

	target_link_libraries (compizconfig_gnome_integration_gsettings_setting_factory
			       compizconfig_gnome_integration_constants
			       compizconfig_gnome_integrated_setting
			       compizconfig_gnome_integration_gsettings_integrated_setting
			       compizconfig
			       gsettings_backend_shared
			       ${COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATIONLIBRARIES})

    endif (COMPIZCONFIG_GNOME_GSETTINGS_INTEGRATION_FOUND)

endif (USE_GCONF)
