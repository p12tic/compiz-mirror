project (compizconfig-python)

find_package (Compiz	   REQUIRED)
find_package (CompizConfig REQUIRED)
find_package (PythonInterp REQUIRED)
find_package (PythonLibs   REQUIRED)

include (CompizCommon)

set (CCS_PYTHON_REQUIRES
     libcompizconfig
     glib-2.0
    )

compiz_pkg_check_modules (CCS_PYTHON REQUIRED ${CCS_PYTHON_REQUIRES})

find_program (HAVE_PYREX pyrexc)

if (NOT HAVE_PYREX)
    message (FATAL_ERROR "Couldn't find pyrexc")
endif ()

compiz_configure_file (${CMAKE_SOURCE_DIR}/compizconfig-python.pc.in
		       ${CMAKE_BINARY_DIR}/compizconfig-python.pc)

add_subdirectory (src)

install (FILES ${CMAKE_BINARY_DIR}/compizconfig-python.pc
	 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
