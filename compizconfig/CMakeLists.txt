set (COMPIZ_CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${COMPIZ_CMAKE_MODULE_PATH})
include (CompizDefaults)

set (_LIBCOMPIZCONFIG_INTERNAL ON)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libcompizconfig/cmake ${CMAKE_MODULE_PATH})
set (COMPIZCONFIG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libcompizconfig/include)
set (COMPIZCONFIG_LIBRARY_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libcompizconfig)

add_subdirectory (libcompizconfig)
add_subdirectory (compizconfig-python)
add_subdirectory (ccsm)
add_subdirectory (gconf)
add_subdirectory (gsettings)
