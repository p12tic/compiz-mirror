find_package (CompizConfig REQUIRED)
include (LibCompizConfigCommon)

# Hack
set (CMAKE_PROJECT_NAME compizconfig-python)

file (READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION COMPIZ_RELEASE_VERSION LIMIT 12 OFFSET 0)
string (STRIP ${COMPIZ_RELEASE_VERSION} COMPIZ_RELEASE_VERSION)

set (VERSION ${COMPIZ_RELEASE_VERSION})

add_custom_target (python ALL
		    COMMAND python setup.py build
		    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		    COMMENT "Generating Python Bytecode")

add_custom_target (install
		   COMMAND python setup.py install --prefix=${CMAKE_INSTALL_PREFIX}
		   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		   COMMENT "Installing Python Files")

add_custom_target (clean
		   COMMAND python setup.py clean
		   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		   COMMENT "Cleaning Python Files")

add_custom_target (uninstall
		   COMMAND python setup.py uninstall
		   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		   COMMENT "Uninstalling Python Files")

add_dependencies (python compizconfig)

find_program (PYTHON_EXECUTABLE python)
mark_as_advanced (FORCE PYTHON_EXECUTABLE)

file (GLOB PYTHON_TESTS "tests/test_*.py")

if (PYTHON_EXECUTABLE)
	foreach (PYTHON_TEST_FILE ${PYTHON_TESTS})
		get_filename_component (PYTHON_TEST ${PYTHON_TEST_FILE} NAME_WE)
		message (STATUS "Adding test " ${PYTHON_TEST})
		add_test (NAME ${PYTHON_TEST} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND ${PYTHON_EXECUTABLE} -m unittest ${PYTHON_TEST})
	endforeach (PYTHON_TEST_FILE)
endif (PYTHON_EXECUTABLE)

compiz_print_configure_header ("CompizConfig Python")
compiz_print_configure_footer ()

compiz_add_git_dist ()
compiz_add_distcheck ()
compiz_add_release ()
compiz_add_release_signoff ()


