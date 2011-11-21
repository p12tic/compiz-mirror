include_directories (${COMPIZ_INCLUDE_DIRS}
		     ${COMPIZ_TEST_TIMER_INCLUDE_DIRS}
		     ${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated
		     ${compiz_SOURCE_DIR}/src
		     ${COMPIZ_TIMER_TEST_BASE_DIR})

link_directories (${COMPIZ_LIBRARY_DIRS})

add_executable (compiz_paintscheduler_test
		../../src/paintscheduler.cpp
		${compiz_SOURCE_DIR}/src/timer.cpp
		${compiz_SOURCE_DIR}/src/timeouthandler.cpp
		test-paintscheduler.cpp)

target_link_libraries (compiz_paintscheduler_test
		       ${COMPIZ_LIBRARIES} m pthread dl)

add_test (test-paintscheduler
	  ${CMAKE_CURRENT_BINARY_DIR}/compiz_paintscheduler_test)
