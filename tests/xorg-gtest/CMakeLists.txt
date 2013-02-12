set (COMPIZ_LD_LIBRARY_PATH ${CMAKE_BINARY_DIR}/src CACHE STRING "" FORCE)
set (COMPIZ_BINARY ${CMAKE_BINARY_DIR}/src/compiz CACHE STRING "" FORCE)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/src/compiz-xorg-gtest-config.h.in
		${CMAKE_CURRENT_BINARY_DIR}/src/compiz-xorg-gtest-config.h
		@ONLY)

include_directories (${XORG_SERVER_INCLUDE_XORG_GTEST}
		     ${XORG_SERVER_GTEST_INCLUDES}
                     ${COMPIZ_XORG_SYSTEM_TEST_INCLUDE_DIR}
		     ${CMAKE_CURRENT_BINARY_DIR}/src)

link_directories (${XORG_SERVER_LIBRARY_DIRS})

add_definitions (${XORG_SERVER_GTEST_CFLAGS})

set (_xorg_gtest_all_srcs
     ${XORG_SERVER_GTEST_SRC}/src/xorg-gtest-all.cpp)

set (_xorg_gtest_main_srcs
     ${XORG_SERVER_GTEST_SRC}/src/xorg-gtest_main.cpp)

add_library (xorg_gtest_all STATIC
	     ${_xorg_gtest_all_srcs})

add_library (xorg_gtest_main STATIC
	     ${_xorg_gtest_main_srcs})

add_library (compiz_xorg_gtest_system_test STATIC
	     ${CMAKE_CURRENT_SOURCE_DIR}/src/compiz-xorg-gtest.cpp)

target_link_libraries (xorg_gtest_all
		       ${GTEST_BOTH_LIBRARIES}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${XORG_SERVER_GTEST_LIBRARIES})

target_link_libraries (xorg_gtest_main
		       ${GTEST_BOTH_LIBRARIES}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${XORG_SERVER_GTEST_LIBRARIES})

target_link_libraries (compiz_xorg_gtest_system_test
		       xorg_gtest_all
		       xorg_gtest_main
		       ${GTEST_BOTH_LIBRARIES}
		       ${CMAKE_THREAD_LIBS_INIT}
		       ${XORG_SERVER_LIBRARIES}
		       ${X11_XI_LIBRARIES})
