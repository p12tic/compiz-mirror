include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR} 
)

add_executable( 
  compiz_option_test
  
  ${CMAKE_CURRENT_SOURCE_DIR}/option.cpp
)

target_link_libraries( 
  compiz_option_test 
  
  compiz_core
  
  ${GTEST_BOTH_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_test( compiz_option_test compiz_option_test )
