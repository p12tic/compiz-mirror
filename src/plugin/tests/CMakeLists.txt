include(CompizTesting)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR} 

  ${compiz_SOURCE_DIR}/include
  ${compiz_SOURCE_DIR}/src
  ${compiz_SOURCE_DIR}/tests/shared
)

add_definitions (
    -DHAVE_CONFIG_H
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DSHAREDIR=\\\"${compiz_sharedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)


add_executable( 
  compiz_plugin_test
  
  ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin.cpp
)

target_link_libraries( 
  compiz_plugin_test 
  
  compiz_plugin
  compiz_core
  
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
)

compiz_discover_tests ( compiz_plugin_test COVERAGE compiz_plugin compiz_core )

