include (CompizGSettings)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}
		     ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../src
                     ${CMAKE_CURRENT_SOURCE_DIR}/../../libcompizconfig/tests
		     ${CMAKE_CURRENT_SOURCE_DIR}/../../mocks/libcompizconfig
		     ${CMAKE_CURRENT_SOURCE_DIR}/../../tests)

pkg_check_modules (COMPIZCONFIG_TEST_GSETTINGS libcompizconfig)

link_directories (${CMAKE_CURRENT_BINARY_DIR}/../../libcompizconfig/tests)

add_library (compizconfig_ccs_gsettings_backend_mock STATIC
	     ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_backend_mock.cpp
	     ${CMAKE_CURRENT_SOURCE_DIR}/ccs_gsettings_backend_mock.h)

add_executable (compizconfig_test_gsettings
		${CMAKE_CURRENT_SOURCE_DIR}/wrap_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/wrap_gsettings.h
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_tests.h)

add_executable (compizconfig_test_gsettings_conformance
		${CMAKE_CURRENT_SOURCE_DIR}/test_gsettings_conformance.cpp)

target_link_libraries (compizconfig_test_gsettings
		       gsettings_backend_shared
		       compizconfig_ccs_setting_mock
		       compizconfig_ccs_gsettings_backend_mock
		       ${COMPIZCONFIG_TEST_GSETTINGS_LIBRARIES}
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_MAIN_LIBRARY})

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/backend-conformance-config.h.in
		${CMAKE_CURRENT_BINARY_DIR}/backend-conformance-config.h)

add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/org.compiz.mock.gschema.xml
		    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/org.compiz.mock.gschema.xml ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/org.compiz.mock.gschema.xml
		    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/org.compiz.mock.gschema.xml
		    VERBATIM)

add_custom_target (compiz_gsettings_mock_schema ALL
		   DEPENDS ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/org.compiz.mock.gschema.xml)

add_gsettings_schema_to_recompilation_list (compiz_gsettings_mock_schema)

target_link_libraries (compizconfig_test_gsettings_conformance
		       gsettings_backend_shared
		       compizconfig_ccs_setting_mock
		       compizconfig_ccs_plugin_mock
		       compizconfig_ccs_context_mock
		       ${COMPIZCONFIG_TEST_GSETTINGS_LIBRARIES}
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_MAIN_LIBRARY})

compiz_discover_tests (compizconfig_test_gsettings)
compiz_discover_tests (compizconfig_test_gsettings_conformance)
