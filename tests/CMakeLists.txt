include (CompizCommon)
include (FindPkgConfig)

pkg_check_modules (COMPIZ_TEST_TIMER glibmm-2.4)

if (COMPIZ_TEST_TIMER_FOUND)

	set (COMPIZ_TIMER_TEST_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

	add_subdirectory (callbacks)
	add_subdirectory (diffs)
	add_subdirectory (set-values)
	add_subdirectory (while-calling)

endif (COMPIZ_TEST_TIMER_FOUND)
