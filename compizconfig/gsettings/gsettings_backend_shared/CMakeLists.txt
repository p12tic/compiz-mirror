include (FindPkgConfig)

pkg_check_modules (GSETTINGS glib-2.0>=2.20.0 gio-2.0>=2.25.0)

include_directories (${GSETTINGS_INCLUDE_DIRS}
		     ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_library (gsettings_backend_shared STATIC
             ${CMAKE_CURRENT_SOURCE_DIR}/gsettings_constants.c)

target_link_libraries (gsettings_backend_shared
		       ${GSETTINGS_LIBRARIES})
