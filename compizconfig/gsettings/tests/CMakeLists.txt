include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable (compizconfig_test_gsettings
		${CMAKE_CURRENT_SOURCE_DIR}/gsettings_mocks.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/gsettings_mocks.h
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.h)

target_link_libraries (compizconfig_test_gsettings
		       gsettings_backend_shared
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_MAIN_LIBRARY})

gtest_add_tests (${CMAKE_CURRENT_BINARY_DIR}/compizconfig_test_gsettings ""
		 ${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.cpp)
