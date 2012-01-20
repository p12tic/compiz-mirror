find_package (Compiz REQUIRED)

include (CompizPlugin)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/constrain-to-workarea/include/)
add_subdirectory (src/constrain-to-workarea)

compiz_plugin (place LIBRARIES compiz_place_constrain_to_workarea)
