include (CompizBcop)

compiz_module (core string LIBRARIES ${Boost_LIBRARIES}
		     	   INCDIRS ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

compiz_module (core logmessage LIBRARIES ${Boost_LIBRARIES}
		     	       INCDIRS ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

compiz_module (core timer PKGDEPS glibmm-2.4
		     LIBRARIES ${Boost_LIBRARIES}
		     INCDIRS ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

compiz_module (core pluginclasshandler MODULES core_string core_logmessage
		     LIBRARIES ${Boost_LIBRARIES}
		     INCDIRS ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

compiz_add_bcop_targets (
    core
    ${compiz_BINARY_DIR}/generated/core.xml.in
    _bcop_sources
)

get_property (CORE_MOD_INCLUDE_DIRS
	      GLOBAL
	      PROPERTY CORE_MOD_INCLUDE_DIRS)

include_directories (
    ${compiz_SOURCE_DIR}/include
    ${compiz_BINARY_DIR}
    ${compiz_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMPIZ_INCLUDE_DIRS}
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
    ${CORE_MOD_INCLUDE_DIRS}
)

add_definitions (
    -DHAVE_CONFIG_H
    -DPLUGINDIR=\\\"${compiz_plugindir}\\\"
    -DSHAREDIR=\\\"${compiz_sharedir}\\\"
    -DMETADATADIR=\\\"${compiz_metadatadir}\\\"
)

get_property (CORE_MOD_LIBRARY_DIRS
	      GLOBAL
	      PROPERTY CORE_MOD_LIBRARY_DIRS)

link_directories (
    ${COMPIZ_LINK_DIRS}
    ${CORE_MOD_LIBRARY_DIRS}
)

add_executable (compiz
    region.cpp
    atoms.cpp
    main.cpp
    actions.cpp
    screen.cpp
    window.cpp
    action.cpp
    option.cpp
    match.cpp
    event.cpp
    plugin.cpp
    session.cpp
    output.cpp
    rect.cpp
    size.cpp
    point.cpp
    windowgeometry.cpp
    icon.cpp
    modifierhandler.cpp
    propertywriter.cpp
    eventsource.cpp
    stackdebugger.cpp
    ${_bcop_sources}
)

# workaround for build race
add_dependencies (compiz core-xml-file)

get_property (CORE_MOD_LIBRARIES
	      GLOBAL
	      PROPERTY CORE_MOD_LIBRARIES)

target_link_libraries (
    compiz ${COMPIZ_LIBRARIES} m pthread dl
    ${CORE_MOD_LIBRARIES}
)

foreach (_lib ${CORE_MOD_LIBRARIES})
    target_link_libraries (
	compiz ${_lib}
    )
endforeach (_lib)

install (
    TARGETS compiz
    DESTINATION ${COMPIZ_DESTDIR}${exec_prefix}
)
