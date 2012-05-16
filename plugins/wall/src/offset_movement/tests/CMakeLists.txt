pkg_check_modules (
  COMPIZ
  REQUIRED
  glibmm-2.4 glib-2.0 compiz
)

find_library (GMOCK_LIBRARY gmock)
find_library (GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
  message ("Google Mock and Google Test not found - cannot build tests!")
  set (COMPIZ_BUILD_TESTING OFF)
endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

include_directories (${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${COMPIZ_INCLUDE_DIRS})

link_directories (${COMPIZ_LIBRARY_DIRS})

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
