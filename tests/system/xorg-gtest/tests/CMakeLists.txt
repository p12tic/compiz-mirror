pkg_check_modules (X11_XI x11 xi)

if (BUILD_XORG_GTEST AND X11_XI_FOUND)

    include_directories (${compiz_SOURCE_DIR}/tests/shared
                         ${COMPIZ_XORG_SYSTEM_TEST_INCLUDE_DIR}
			 ${X11_INCLUDE_DIRS}
			 ${XORG_SERVER_INCLUDE_XORG_GTEST}
			 ${XORG_SERVER_GTEST_SRC}
			 ${GTEST_INCLUDE_DIRS})

    link_directories (${X11_XI_LIBRARY_DIRS}
                      ${compiz_BINARY_DIR}/tests/shared/src)

    add_executable (compiz_xorg_gtest_test_window_stacking
		    ${CMAKE_CURRENT_SOURCE_DIR}/compiz_xorg_gtest_test_window_stacking.cpp)

    add_executable (compiz_xorg_gtest_test_icccm
		    ${CMAKE_CURRENT_SOURCE_DIR}/compiz_xorg_gtest_icccm.cpp)

    add_executable (compiz_xorg_gtest_test_shape_handling
		    ${CMAKE_CURRENT_SOURCE_DIR}/compiz_xorg_gtest_test_shape_handling.cpp)

    set (COMPIZ_XORG_GTEST_LIBRARIES
	 compiz_xorg_gtest_system_test
	 xorg_gtest_all
	 xorg_gtest_main
         ${GTEST_BOTH_LIBRARIES}
	 ${XORG_SERVER_LIBRARIES}
	 ${X11_XI_LIBRARIES}
	 ${CMAKE_THREAD_LIBS_INIT})

    target_link_libraries (compiz_xorg_gtest_test_window_stacking
			   ${COMPIZ_XORG_GTEST_LIBRARIES})

    compiz_discover_tests (compiz_xorg_gtest_test_window_stacking)

    target_link_libraries (compiz_xorg_gtest_test_icccm
                           compiz_gtest_shared_async_task
			   ${COMPIZ_XORG_GTEST_LIBRARIES})

    compiz_discover_tests (compiz_xorg_gtest_test_icccm)

    target_link_libraries (compiz_xorg_gtest_test_shape_handling
			   ${COMPIZ_XORG_GTEST_LIBRARIES})

    compiz_discover_tests (compiz_xorg_gtest_test_shape_handling)

endif (BUILD_XORG_GTEST AND X11_XI_FOUND)
