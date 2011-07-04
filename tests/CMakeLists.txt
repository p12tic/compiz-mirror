include (CompizCommon)
include (FindPkgConfig)

pkg_check_modules (COMPIZ_TEST_TIMER glibmm-2.4)

if (COMPIZ_TEST_TIMER_FOUND)

	link_directories (${COMPIZ_TEST_TIMER_LINK_DIRS})

	include_directories (${COMPIZ_TEST_TIMER_INCLUDE_DIRS}
			     ${compiz_SOURCE_DIR}/include
			     ${compiz_BINARY_DIR}
			     ${compiz_BINARY_DIR}/generated
			     ${compiz_SOURCE_DIR}/src)

	add_executable (test-timer
			test-timer.cpp
			${compiz_SOURCE_DIR}/src/timer.cpp
			${compiz_SOURCE_DIR}/src/timeouthandler.cpp)

	target_link_libraries (test-timer
			       ${COMPIZ_TEST_TIMER_LIBRARIES})	

endif (COMPIZ_TEST_TIMER_FOUND)
