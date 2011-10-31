#include (CompizCommon)
#include (FindPkgConfig)

#set (COMPIZ_TEST_TIMER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#set (COMPIZ_TEST_TIMER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

#include_directories (${COMPIZ_TIMER_SOURCE_DIR}
#		     ${COMPIZ_TIMER_INCLUDE_DIR})

#add_library (timer_test_internal STATIC
#	     test-timer.cpp)

compiz_test_base (core timer)
add_subdirectory (callbacks)
add_subdirectory (diffs)
add_subdirectory (set-values)
add_subdirectory (while-calling)
