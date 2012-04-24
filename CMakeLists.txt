find_package (Compiz REQUIRED)

include (CompizPlugin)

add_subdirectory (src/offset_movement)
include_directories (src/offset_movement/include)

compiz_plugin (wall PLUGINDEPS composite opengl mousepoll PKGDEPS cairo cairo-xlib-xrender LIBRARIES compiz_wall_offset_movement dl)
