
include_directories(${GTEST_INCLUDE_DIRS} ..)
add_definitions(-DMOCK_COMPWINDOW)
set(exe "compiz_opengl_test_windowstack")
add_executable(${exe} test-windowstack.cpp ../windowstack.cpp)
target_link_libraries(${exe} compiz_core ${GTEST_BOTH_LIBRARIES})
add_test(OpenGLWindowStackTests ${exe})

