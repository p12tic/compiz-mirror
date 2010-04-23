
set (COMPIZ_PREFIX ${CMAKE_INSTALL_PREFIX})
set (COMPIZ_INCLUDEDIR ${includedir})
set (COMPIZ_LIBDIR ${libdir})
set (COMPIZ_DATADIR ${datadir})
set (COMPIZ_METADATADIR ${datadir}/compiz)
set (COMPIZ_IMAGEDIR ${datadir}/compiz/images)
set (COMPIZ_PLUGINDIR ${libdir}/compiz)
set (COMPIZ_SYSCONFDIR ${sysconfdir})

list (APPEND COMPIZ_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include)
list (APPEND COMPIZ_INCLUDE_DIRS ${CMAKE_BINARY_DIR})

set (COMPIZ_BCOP_XSLT ${CMAKE_SOURCE_DIR}/xslt/bcop.xslt)

set (COMPIZ_GCONF_SCHEMAS_SUPPORT ${USE_GCONF})
set (COMPIZ_GCONF_SCHEMAS_XSLT ${CMAKE_SOURCE_DIR}/xslt/compiz_gconf_schemas.xslt)

set (COMPIZ_PLUGIN_INSTALL_TYPE "package")

set (_COMPIZ_INTERNAL 1)
