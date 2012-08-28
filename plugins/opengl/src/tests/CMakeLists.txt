
include_directories(${GTEST_INCLUDE_DIRS})
add_definitions(-DOUTPUTWINDOW=MockCompWindow)
add_executable(test-output test-output.cpp ../output.cpp)
target_link_libraries(test-output compiz_core ${GTEST_BOTH_LIBRARIES})
add_test(OutputTests test-output)

