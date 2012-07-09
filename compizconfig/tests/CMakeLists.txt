include_directories (${GTEST_INCLUDE_DIRS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../libcompizconfig/include)
link_directories (${CMAKE_INSTALL_PREFIX}/lib)

add_library (compizconfig_ccs_backend_conformance_test
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig_backend_concept_test.cpp)

target_link_libraries (compizconfig_ccs_backend_conformance_test
		       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${LIBCOMPIZCONFIG_LIBRARIES}
		       compizconfig)
