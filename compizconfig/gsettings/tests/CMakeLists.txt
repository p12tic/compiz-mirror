include_directories (${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../src
                     ${CMAKE_CURRENT_SOURCE_DIR}/../../libcompizconfig/tests
		     ${CMAKE_CURRENT_SOURCE_DIR}/../../mocks/libcompizconfig
		     ${CMAKE_CURRENT_SOURCE_DIR}/../../tests
		     ${CMAKE_SOURCE_DIR}/tests/shared)

pkg_check_modules (COMPIZCONFIG_TEST_GSETTINGS libcompizconfig)

add_library (compizconfig_ccs_gsettings_backend_mock STATIC
	     ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_backend_mock.cpp
	     ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_backend_mock.h)

add_executable (compizconfig_test_gsettings
		${CMAKE_CURRENT_SOURCE_DIR}/wrap_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/wrap_gsettings.h
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.h)

link_directories (${CMAKE_CURRENT_BINARY_DIR}/../../libcompizconfig/tests)

target_link_libraries (compizconfig_test_gsettings
		       gsettings_backend_shared
		       compizconfig_ccs_setting_mock
                       compizconfig_ccs_context_mock
		       compizconfig_ccs_gsettings_backend_mock
		       ${COMPIZCONFIG_TEST_GSETTINGS_LIBRARIES}
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_MAIN_LIBRARY})

compiz_discover_tests (compizconfig_test_gsettings COVERAGE gsettings_backend_shared)
