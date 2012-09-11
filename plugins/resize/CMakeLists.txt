find_package (Compiz REQUIRED)

include (CompizPlugin)

if (NOT COMPIZ_DISABLE_PLUGIN_RESIZE)
    add_subdirectory (src/logic)
    include_directories (src/logic/include)
endif (NOT COMPIZ_DISABLE_PLUGIN_RESIZE)

compiz_plugin(resize PLUGINDEPS composite opengl LIBRARIES resize_logic)
