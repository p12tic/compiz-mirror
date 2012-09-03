
include_directories(${GTEST_INCLUDE_DIRS} ..)
set(exe "compiz_opengl_test_fsregion")
add_executable(${exe} test-fsregion.cpp ../fsregion.cpp)
target_link_libraries(${exe} compiz_core ${GTEST_BOTH_LIBRARIES})
compiz_discover_tests(${exe})

