include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)

pkg_check_modules (COMPIZCONFIG_TEST_GSETTINGS libcompizconfig)

add_executable (compizconfig_test_gsettings
		${CMAKE_CURRENT_SOURCE_DIR}/gsettings_mocks.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/gsettings_mocks.h
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.h)

target_link_libraries (compizconfig_test_gsettings
		       gsettings_backend_shared
		       ${COMPIZCONFIG_TEST_GSETTINGS_LIBRARIES}
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_MAIN_LIBRARY})

compiz_discover_tests (compizconfig_test_gsettings)
