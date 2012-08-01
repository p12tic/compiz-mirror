 include_directories (${GTEST_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../mocks/libcompizconfig)
link_directories (${CMAKE_INSTALL_PREFIX}/lib)
link_directories (${CMAKE_CURRENT_BINARY_DIR}/../../mocks/libcompizconfig)

add_executable (compizconfig_test_ccs_object
		${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_test_ccs_object.cpp)

add_executable (compizconfig_test_ccs_context
		${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_test_ccs_context.cpp)

add_executable (compizconfig_test_ccs_plugin
		${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_test_ccs_plugin.cpp)

add_executable (compizconfig_test_ccs_setting
		${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_test_ccs_setting.cpp)

if (HAVE_PROTOBUF)
    set (LIBCOMPIZCONFIG_LIBRARIES
	 ${LIBCOMPIZCONFIG_LIBRARIES}
	 protobuf)
endif (HAVE_PROTOBUF)

target_link_libraries (compizconfig_test_ccs_object
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (compizconfig_test_ccs_context
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       compizconfig_ccs_context_mock
		       compizconfig_ccs_plugin_mock
		       compizconfig_ccs_setting_mock)

target_link_libraries (compizconfig_test_ccs_plugin
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       compizconfig_ccs_context_mock
		       compizconfig_ccs_plugin_mock
		       compizconfig_ccs_setting_mock)

target_link_libraries (compizconfig_test_ccs_setting
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       compizconfig_ccs_context_mock
		       compizconfig_ccs_plugin_mock
		       compizconfig_ccs_setting_mock)

compiz_discover_tests (compizconfig_test_ccs_object COVERAGE compizconfig)
compiz_discover_tests (compizconfig_test_ccs_context COVERAGE compizconfig_ccs_context_mock)
compiz_discover_tests (compizconfig_test_ccs_plugin COVERAGE compizconfig_ccs_plugin_mock)
compiz_discover_tests (compizconfig_test_ccs_setting COVERAGE compizconfig_ccs_setting_mock)
