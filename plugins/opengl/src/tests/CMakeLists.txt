
include_directories(${GTEST_INCLUDE_DIRS})
add_definitions(-DMOCK_COMPWINDOW)
set(exe "compiz_opengl_test_output")
add_executable(${exe} test-output.cpp ../output.cpp)
target_link_libraries(${exe} compiz_core ${GTEST_BOTH_LIBRARIES})
add_test(OpenGLOutputTests ${exe})

