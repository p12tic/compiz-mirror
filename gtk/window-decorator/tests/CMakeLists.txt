include (CompizGSettings)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../
		     ${CMAKE_CURRENT_BINARY_DIR})

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/compiz_gwd_tests.h.in
                ${CMAKE_CURRENT_BINARY_DIR}/compiz_gwd_tests.h)

pkg_check_modules (COMPIZ_TEST_GTK_WINDOW_DECORATOR
		   glib-2.0>=2.28
                   gio-2.0>=2.25.0)

if (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)

    link_directories (${CMAKE_CURRENT_BINARY_DIR}
		      ${CMAKE_CURRENT_BINARY_DIR}/..)

    add_library (gtk_window_decorator_mock_settings_storage STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/compiz_gwd_mock_settings_storage.cpp)

    target_link_libraries (gtk_window_decorator_mock_settings_storage
			   gtk_window_decorator_settings_storage_interface)

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
			   gtk_window_decorator_settings_storage_gsettings
		           gtk_window_decorator_mock_settings
			   gtk_window_decorator_mock_settings_writable
			   gtk_window_decorator_mock_settings_storage
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

    set (_desktop_gschema_name org.gnome.desktop.wm.preferences)
    set (_desktop_gschema_filename ${_desktop_gschema_name}.gschema.xml)
    set (_desktop_gschema_filepath ${CMAKE_CURRENT_SOURCE_DIR}/${_desktop_gschema_filename})
    set (_desktop_gschema_generated_location ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/${_desktop_gschema_filename})

    add_custom_command (OUTPUT ${_desktop_gschema_generated_location}
		        COMMAND cp -r ${_desktop_gschema_filepath} ${_desktop_gschema_generated_location}
		        DEPENDS ${_desktop_gschema_filepath}
			VERBATIM)

    add_custom_target (compiz_gwd_gsettings_org_gnome_desktop_wm_preferences_schema ALL
		       DEPENDS ${_desktop_gschema_generated_location})

    add_gsettings_schema_to_recompilation_list (compiz_gwd_gsettings_org_gnome_desktop_wm_preferences_schema)

    set (_mutter_gschema_name org.gnome.desktop.wm.preferences)
    set (_mutter_gschema_filename ${_mutter_gschema_name}.gschema.xml)
    set (_mutter_gschema_filepath ${CMAKE_CURRENT_SOURCE_DIR}/${_mutter_gschema_filename})
    set (_mutter_gschema_generated_location ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/${_mutter_gschema_filename})

    add_custom_command (OUTPUT ${_mutter_gschema_generated_location}
		        COMMAND cp -r ${_mutter_gschema_filepath} ${_mutter_gschema_generated_location}
		        DEPENDS ${_mutter_gschema_filepath}
			VERBATIM)

    add_custom_target (compiz_gwd_gsettings_org_gnome_mutter_schema ALL
		       DEPENDS ${_mutter_gschema_generated_location})

    add_gsettings_schema_to_recompilation_list (compiz_gwd_gsettings_org_gnome_mutter_schema)

endif (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)
