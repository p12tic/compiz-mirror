find_package (Compiz REQUIRED)

include (CompizPlugin)

pkg_check_modules (RSVG librsvg-2.0>=2.36.2)

if (RSVG_FOUND)
    set (HAVE_RSVG_23_6_2 TRUE)
else (RSVG_FOUND)
    set (HAVE_RSVG_23_6_2 FALSE)
endif (RSVG_FOUND)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
		${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

compiz_plugin(imgsvg PLUGINDEPS composite opengl PKGDEPS cairo>=1.0 cairo-xlib librsvg-2.0>=2.14.0)
