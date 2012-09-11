find_package (Compiz REQUIRED)
include (CompizPlugin)

if (NOT COMPIZ_DISABLE_PLUGIN_EXPO)
    add_subdirectory (src/click_threshold)
    include_directories (src/click_threshold/include)
endif (NOT COMPIZ_DISABLE_PLUGIN_EXPO)
compiz_plugin (expo PLUGINDEPS composite opengl LIBRARIES compiz_expo_click_threshold)
