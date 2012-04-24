# Build Google Test and make its headers known
find_package (GTest)

if (NOT GTEST_FOUND)

    # Check for google test and build it locally
    set (GTEST_ROOT_DIR
	"/usr/src/gtest" # Default value, adjustable by user with e.g., ccmake
	CACHE
	PATH
	"Path to Google test srcs"
    )

    find_path (GTEST_INCLUDE_DIR gtest/gtest.h)

    if (GTEST_INCLUDE_DIR)
	#FIXME - hardcoded is bad!
	add_subdirectory (${GTEST_ROOT_DIR}
		      gtest)
    endif(GTEST_INCLUDE_DIR)

    set (GTEST_BOTH_LIBRARIES gtest gtest_main)
    set (GTEST_FOUND TRUE)

endif (NOT GTEST_FOUND)

find_library (GMOCK_LIBRARY gmock)
find_library (GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
  message ("Google Mock and Google Test not found - cannot build tests!")
  set (COMPIZ_BUILD_TESTING OFF)
endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

include_directories (${GTEST_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable (compiz_test_wall_offset_movement
                ${CMAKE_CURRENT_SOURCE_DIR}/test-wall-offset-movement.cpp)

target_link_libraries (compiz_test_wall_offset_movement
                       compiz_wall_offset_movement
                       ${GTEST_BOTH_LIBRARIES}
		       ${GMOCK_LIBRARY}
		       ${GMOCK_MAIN_LIBRARY}
		       ${CMAKE_THREAD_LIBS_INIT} # Link in pthread. 
                       )

gtest_add_tests (compiz_test_wall_offset_movement "" ${CMAKE_CURRENT_SOURCE_DIR}/test-wall-offset-movement.cpp)
