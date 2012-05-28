find_package (Compiz REQUIRED)

include (CompizPlugin)

set (CMAKE_CXX_FLAGS -std=c++0x)

include_directories (src/grabhandler/include)

compiz_plugin (grid PLUGINDEPS composite opengl LIBRARIES compiz_grid_grab_handler)

add_subdirectory (src/grabhandler)
