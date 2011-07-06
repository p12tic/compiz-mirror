link_directories (${COMPIZ_TEST_TIMER_LINK_DIRS})

include_directories (${COMPIZ_TEST_TIMER_INCLUDE_DIRS}
		     ${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated
		     ${compiz_SOURCE_DIR}/src
		     ${COMPIZ_TIMER_TEST_BASE_DIR})

set (OBJECT CompTimerTestSetValues)
add_definitions (-DOBJECT=${OBJECT})

add_executable (test-timer-set-values
		test-timer-set-values.cpp
		../test-timer.cpp 
		${compiz_SOURCE_DIR}/src/timer.cpp
		${compiz_SOURCE_DIR}/src/timeouthandler.cpp)

target_link_libraries (test-timer-set-values
		       ${COMPIZ_TEST_TIMER_LIBRARIES})

add_test (test-timer-set-values
	  ${CMAKE_CURRENT_BINARY_DIR}/test-timer-set-values)
