include_directories (${GTEST_INCLUDE_DIRS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../libcompizconfig/include)
link_directories (${CMAKE_INSTALL_PREFIX}/lib)

if (HAVE_PROTOBUF)
    set (LIBCOMPIZCONFIG_LIBRARIES
	 ${LIBCOMPIZCONFIG_LIBRARIES}
	 protobuf)
endif (HAVE_PROTOBUF)

add_library (compizconfig_ccs_context_mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_ccs_context_mock.cpp)

add_library (compizconfig_ccs_plugin_mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_ccs_plugin_mock.cpp)

add_library (compizconfig_ccs_setting_mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_ccs_setting_mock.cpp)

add_library (compizconfig_ccs_backend_mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_ccs_backend_mock.cpp)

target_link_libraries (compizconfig_ccs_context_mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (compizconfig_ccs_plugin_mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (compizconfig_ccs_setting_mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (compizconfig_ccs_backend_mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)
