find_package (Compiz REQUIRED)

include (CompizPlugin)

if (NOT COMPIZ_DISABLE_PLUGIN_COMPOSITE)
    include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/pixmapbinding/include)
    link_directories (${CMAKE_CURRENT_BINARY_DIR}/src/pixmapbinding)
    add_subdirectory (src/pixmapbinding)
endif (NOT COMPIZ_DISABLE_PLUGIN_COMPOSITE)

compiz_plugin (composite LIBRARIES compiz_composite_pixmapbinding)
