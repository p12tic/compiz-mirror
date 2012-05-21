include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

add_executable( 
  compiz_wrapsystem_test
  
  ${CMAKE_CURRENT_SOURCE_DIR}/test-wrapsystem.cpp
)

target_link_libraries( 
  compiz_wrapsystem_test
  
  ${GTEST_BOTH_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

gtest_add_tests (${CMAKE_CURRENT_BINARY_DIR}/compiz_wrapsystem_test "" "test-wrapsystem.cpp")
