include (FindPkgConfig)

pkg_check_modules (GSETTINGS_UTIL glib-2.0>=2.20.0
                                  gio-2.0>=2.25.0
				  libcompizconfig)

include_directories (${compiz_SOURCE_DIR}/compizconfig/libcompizconfig/include
		     ${GSETTINGS_UTIL_INCLUDE_DIRS}
		     ${CMAKE_CURRENT_SOURCE_DIR}/../src)

link_directories (${GSETTINGS_UTIL_LIBRARY_DIRS})

add_library (gsettings_backend_shared STATIC
             ${CMAKE_CURRENT_SOURCE_DIR}/gsettings_constants.c
	     ${CMAKE_CURRENT_SOURCE_DIR}/gsettings_util.c)

target_link_libraries (gsettings_backend_shared
                       ${GSETTINGS_UTIL_LIBRARIES})
