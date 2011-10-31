
#if (COMPIZ_TIMER_FOUND)

#	set (COMPIZ_TIMER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
#	set (COMPIZ_TIMER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#	set (COMPIZ_TIMER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

#	include_directories (${compiz_SOURCE_DIR}/include
#			     ${COMPIZ_INCLUDE_DIRS}
#			     ${COMPIZ_TIMER_INCLUDE_DIR})

#	add_library (timer_internal STATIC
#		     timer.cpp)

#	target_link_libraries (
#	    timer_internal ${COMPIZ_TIMER_LIBRARIES} m pthread dl
#	)

#	add_library (timeout_handler_internal STATIC
#		     timeouthandler.cpp)

#	target_link_libraries (
#	    timeout_handler_internal ${COMPIZ_TIMER_LIBRARIES} m pthread dl
#	)

#	add_subdirectory (tests)
#endif (COMPIZ_TIMER_FOUND)
