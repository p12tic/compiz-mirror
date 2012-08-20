include_directories (${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../)

pkg_check_modules (COMPIZ_TEST_GTK_WINDOW_DECORATOR
		   glib-2.0>=2.28
                   gio-2.0>=2.25.0)

if (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)

    link_directories (${CMAKE_CURRENT_BINARY_DIR}
		      ${CMAKE_CURRENT_BINARY_DIR}/..)

    add_library (gtk_window_decorator_mock_settings STATIC
	         ${CMAKE_CURRENT_SOURCE_DIR}/compiz_gwd_mock_settings.cpp)

    target_link_libraries (gtk_window_decorator_mock_settings
		           gtk_window_decorator_settings_interface)

    add_library (gtk_window_decorator_mock_settings_writable STATIC
	         ${CMAKE_CURRENT_SOURCE_DIR}/compiz_gwd_mock_settings_writable.cpp)

    target_link_libraries (gtk_window_decorator_mock_settings_writable
		           gtk_window_decorator_settings_writable_interface)

    add_executable (compiz_test_gwd_settings
		    ${CMAKE_CURRENT_SOURCE_DIR}/test_gwd_settings.cpp)

    target_link_libraries (compiz_test_gwd_settings
		           gtk_window_decorator_settings
		           gtk_window_decorator_mock_settings
			   gtk_window_decorator_mock_settings_writable
			   decoration
		           ${COMPIZ_TEST_GTK_WINDOW_DECORATOR_LIBRARIES}
			   ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
		           ${GTEST_BOTH_LIBRARIES}
			   ${GMOCK_LIBRARY}
		       	   ${GMOCK_MAIN_LIBRARY})

    compiz_discover_tests (compiz_test_gwd_settings
			   COVERAGE
			   gtk_window_decorator_settings
			   gtk_window_decorator_settings_interface)

endif (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)
