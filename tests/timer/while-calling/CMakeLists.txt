link_directories (${COMPIZ_TEST_TIMER_LINK_DIRS})

include_directories (${COMPIZ_TEST_TIMER_INCLUDE_DIRS}
		     ${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated
		     ${compiz_SOURCE_DIR}/src
		     ${COMPIZ_TIMER_TEST_BASE_DIR})

set (OBJECT CompTimerTestSetCalling)
add_definitions (-DOBJECT=${OBJECT})

add_executable (test-timer-set-times-while-calling
		test-timer-set-times-while-calling.cpp
		../test-timer.cpp 
		${compiz_SOURCE_DIR}/src/timer.cpp
		${compiz_SOURCE_DIR}/src/timeouthandler.cpp)

target_link_libraries (test-timer-set-times-while-calling
		       ${COMPIZ_TEST_TIMER_LIBRARIES})

add_test (test-timer-set-times-while-calling
	  ${CMAKE_CURRENT_BINARY_DIR}/test-timer-set-times-while-calling)
