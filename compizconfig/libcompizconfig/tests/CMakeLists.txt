include_directories (${GTEST_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/include)
link_directories (${CMAKE_INSTALL_PREFIX}/lib)

add_library (context-mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/context-mock.cpp)

add_library (plugin-mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/plugin-mock.cpp)

add_library (setting-mock
	     ${CMAKE_CURRENT_SOURCE_DIR}/setting-mock.cpp)

add_executable (test-ccs-object
		${CMAKE_CURRENT_SOURCE_DIR}/test-ccs-object.cpp)

add_executable (test-context
		${CMAKE_CURRENT_SOURCE_DIR}/test-context.cpp)

add_executable (test-plugin
		${CMAKE_CURRENT_SOURCE_DIR}/test-plugin.cpp)

add_executable (test-setting
		${CMAKE_CURRENT_SOURCE_DIR}/test-setting.cpp)

if (HAVE_PROTOBUF)
    set (LIBCOMPIZCONFIG_LIBRARIES
	 ${LIBCOMPIZCONFIG_LIBRARIES}
	 protobuf)
endif (HAVE_PROTOBUF)

target_link_libraries (context-mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (plugin-mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (setting-mock
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (test-ccs-object
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)

target_link_libraries (test-context
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       context-mock
		       plugin-mock
		       setting-mock)

target_link_libraries (test-plugin
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       context-mock
		       plugin-mock
		       setting-mock)

target_link_libraries (test-setting
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig
		       context-mock
		       plugin-mock
		       setting-mock)

compiz_discover_tests (test-ccs-object)
compiz_discover_tests (test-context)
compiz_discover_tests (test-plugin)
compiz_discover_tests (test-setting)
