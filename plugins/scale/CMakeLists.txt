find_package (Compiz REQUIRED)

include (CompizPlugin)

set (CMAKE_CXX_FLAGS -std=c++0x)

compiz_plugin(scale PLUGINDEPS composite opengl)

