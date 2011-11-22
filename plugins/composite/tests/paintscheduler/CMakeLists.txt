include (FindPkgConfig)

pkg_check_modules (COMPIZ_PAINTSCHEDULER REQUIRED dri libdrm glibmm-2.4)

find_library( DRM_LIBRARY drm PATHS /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} NO_DEFAULT_PATH )

include_directories (${COMPIZ_INCLUDE_DIRS}
		     ${COMPIZ_PAINTSCHEDULER_INCLUDE_DIRS}
		     ${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated
		     ${compiz_SOURCE_DIR}/src
		     ../../include
		     ../../src)

message (${DRM_LIBRARY})

# FIXME: Hardcoding drm like this is stupid
# but due to a bug in CMake we have to do it
add_executable (compiz_paintscheduler_test
		../../src/paintscheduler.cpp
		${compiz_SOURCE_DIR}/src/timer.cpp
		${compiz_SOURCE_DIR}/src/timeouthandler.cpp
		test-paintscheduler.cpp
		test-paintscheduler-set-drmvblanktype.c)

target_link_libraries (compiz_paintscheduler_test
		       ${COMPIZ_LIBRARIES} m pthread dl ${DRM_LIBRARY})

set_target_properties (compiz_paintscheduler_test PROPERTIES
			LINK_FLAGS "-L/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}")

add_test (test-paintscheduler
	  ${CMAKE_CURRENT_BINARY_DIR}/compiz_paintscheduler_test)
