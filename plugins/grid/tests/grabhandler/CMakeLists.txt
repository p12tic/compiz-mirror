include_directories (${compiz_INCLUDE_DIRS} ${CMAKE_CURENT_SOURCE_DIR}/../../src)

add_executable (test-grid-grab-handler
		test-grid-grab-handler.cpp
		../../src/grabhandler.cpp)

target_link_libraries (test-grid-grab-handler pthread dl)

add_test (test-grid-grab-handler-test test-grid-grab-handler)
