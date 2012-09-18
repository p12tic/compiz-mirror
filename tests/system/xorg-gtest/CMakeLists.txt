include (FindPkgConfig)

pkg_check_modules (XORG_SERVER xorg-server x11)

if (XORG_SERVER_FOUND)

    foreach (_includedir ${XORG_SERVER_INCLUDE_DIRS})
	set (_include_xorg "/include/xorg")
	string (FIND ${_includedir} ${_include_xorg} _found_include_xorg)
	if (NOT _found_include_xorg EQUAL -1)
	    set (XORG_SERVER_INCLUDE_XORG_GTEST ${_includedir}/gtest)
	    string (LENGTH ${_include_xorg} _include_xorg_length)
	    string (LENGTH ${_includedir} _found_include_xorg_length)
	    math (EXPR _xorg_prefix_length "${_found_include_xorg_length} - ${_include_xorg_length}")
	    string (SUBSTRING ${_includedir} 0 ${_xorg_prefix_length} _xorg_prefix)
	    set (XORG_SERVER_GTEST_SRC ${_xorg_prefix}/src/xorg-gtest)
	endif (NOT _found_include_xorg EQUAL -1)
    endforeach ()

    set (BUILD_XORG_GTEST TRUE)

    if (XORG_SERVER_GTEST_SRC)
	message (STATUS "Found xorg-gtest sources at " ${XORG_SERVER_GTEST_SRC})
    else (XORG_SERVER_GTEST_SRC)
	set (BUILD_XORG_GTEST FALSE)
	message (WARNING "Could not found xorg-gtest sources, not building
			  xorg-gtest tests")
    endif (XORG_SERVER_GTEST_SRC)

    if (XORG_SERVER_INCLUDE_XORG_GTEST)
	message (STATUS "Found xorg-gtest includes at " ${XORG_SERVER_INCLUDE_XORG_GTEST})
    else (XORG_SERVER_INCLUDE_XORG_GTEST)
	set (BUILD_XORG_GTEST FALSE)
	message (WARNING "Could not found xorg-gtest includes, not building
			  xorg-gtest tests")
    endif (XORG_SERVER_INCLUDE_XORG_GTEST)

    if (BUILD_XORG_GTEST)

	include_directories (${XORG_SERVER_INCLUDE_DIRS}
			     ${GTEST_INCLUDE_DIRS}
			     ${XORG_SERVER_INCLUDE_XORG_GTEST}
			     ${XORG_SERVER_GTEST_SRC})

	link_directories (${XORG_SERVER_LIBRARY_DIRS})

	# This actually includes xorg-gtest-all and the defines
	set (_xorg_gtest_all_srcs
	     ${CMAKE_CURRENT_SOURCE_DIR}/src/xorg-gtest-init.cpp)

	set (_xorg_gtest_main_srcs
	     ${XORG_SERVER_GTEST_SRC}/src/xorg-gtest_main.cpp)

	add_library (xorg_gtest_all STATIC
		     ${_xorg_gtest_all_srcs})

	add_library (xorg_gtest_main STATIC
		     ${_xorg_gtest_main_srcs})

	target_link_libraries (xorg_gtest_all
			       ${GTEST_BOTH_LIBRARIES}
			       ${CMAKE_THREAD_LIBS_INIT}
			       ${XORG_SERVER_LIBRARIES})

	target_link_libraries (xorg_gtest_main
			       ${GTEST_BOTH_LIBRARIES}
			       ${CMAKE_THREAD_LIBS_INIT}
			       ${XORG_SERVER_LIBRARIES})
    endif (BUILD_XORG_GTEST)

endif (XORG_SERVER_FOUND)
