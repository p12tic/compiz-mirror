find_package (Compiz REQUIRED)

include (CompizPlugin)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/pixmapbinder/include)
link_directories (${CMAKE_CURRENT_BINARY_DIR}/src/pixmapbinder)

compiz_plugin (composite LIBRARIES compiz_composite_pixmapbinder)

add_subdirectory (src/pixmapbinder)
