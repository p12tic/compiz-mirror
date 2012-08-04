include (FindPkgConfig)

pkg_check_modules (GSETTINGS_UTIL glib-2.0>=2.28
                                  gio-2.0>=2.25.0)

if (GSETTINGS_UTIL_FOUND)

    set (GSETTINGS_UTIL_LIB_INCLUDES
	 ${compiz_SOURCE_DIR}/compizconfig/libcompizconfig/include
	 ${GSETTINGS_UTIL_INCLUDE_DIRS}
	 ${CMAKE_CURRENT_SOURCE_DIR}/../src)

    include_directories (${GSETTINGS_UTIL_LIB_INCLUDES})

    link_directories (${GSETTINGS_UTIL_LIBRARY_DIRS}
		      ${compiz_BINARY_DIR}/compizconfig/libcompizconfig)

    add_library (gsettings_backend_shared SHARED
		 ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_interface_wrapper.c
		 ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_interface.c
		 ${CMAKE_CURRENT_SOURCE_DIR}/gsettings_constants.c
		 ${CMAKE_CURRENT_SOURCE_DIR}/gsettings_util.c)


    target_link_libraries (gsettings_backend_shared
			   ${GSETTINGS_UTIL_LIBRARIES}
			   compizconfig)

    install (TARGETS gsettings_backend_shared
	     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

    #
    # Tell CMake that targets using gsettings_backend_shared should NOT re-import the
    # libraries that gsettings_backend_shared depends on (contains).
    #
    set_target_properties (gsettings_backend_shared PROPERTIES
	LINK_INTERFACE_LIBRARIES ""
    )

endif (GSETTINGS_UTIL_FOUND)
