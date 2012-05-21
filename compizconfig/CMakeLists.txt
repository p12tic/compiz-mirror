set (_COMPIZCONFIG_INTERNAL ON)
set (LIBCOMPIZCONFIG_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libcompizconfig/cmake )
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LIBCOMPIZCONFIG_CMAKE_MODULE_PATH})
set (COMPIZCONFIG_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libcompizconfig/include)
set (COMPIZCONFIG_LIBRARY_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libcompizconfig)

add_subdirectory (libcompizconfig)
#add_subdirectory (compizconfig-python)
#add_subdirectory (ccsm)
#add_subdirectory (gconf)
add_subdirectory (gsettings)

add_custom_target (pyclean)

add_dependencies (pyclean pyclean_ccsm pyclean_compizconfig)
