
cmake_minimum_required (VERSION 2.6)

include (FindPkgConfig)
pkg_search_module (GTK REQUIRED gtk+-3.0)
pkg_search_module (GL REQUIRED gl)

add_executable (untest untest.c)
include_directories (${GTK_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
link_directories (${GTK_LIBRARY_DIRS} ${GL_LIBRARY_DIRECTORIES})
target_link_libraries (untest ${GTK_LIBRARIES} ${GL_LIBRARIES})
