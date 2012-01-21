include (CompizBcop)


add_subdirectory( string )
add_subdirectory( logmessage )
add_subdirectory( timer )
add_subdirectory( pluginclasshandler )
add_subdirectory( point )
add_subdirectory( rect )
add_subdirectory( window )
add_subdirectory( wrapsystem/tests )

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
    ${COMPIZ_INCLUDE_DIRS}
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_INCLUDE_PATH}
    ${CORE_MOD_INCLUDE_DIRS}
    
    ${CMAKE_CURRENT_SOURCE_DIR}
    
    # Module specific include dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/string/include
    ${CMAKE_CURRENT_SOURCE_DIR}/string/src
    
    ${CMAKE_CURRENT_SOURCE_DIR}/logmessage/include
    ${CMAKE_CURRENT_SOURCE_DIR}/logmessage/src
    
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/timer/src
    
    ${CMAKE_CURRENT_SOURCE_DIR}/pluginclasshandler/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pluginclasshandler/src

    ${CMAKE_CURRENT_SOURCE_DIR}/point/include
    ${CMAKE_CURRENT_SOURCE_DIR}/point/src

    ${CMAKE_CURRENT_SOURCE_DIR}/rect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rect/src

    ${CMAKE_CURRENT_SOURCE_DIR}/window/geometry/include
    ${CMAKE_CURRENT_SOURCE_DIR}/window/geometry/src

    ${CMAKE_CURRENT_SOURCE_DIR}/window/geometry-saver/include
    ${CMAKE_CURRENT_SOURCE_DIR}/window/geometry-saver/src

    ${CMAKE_CURRENT_SOURCE_DIR}/window/extents/include
    ${CMAKE_CURRENT_SOURCE_DIR}/window/extents/src

    ${CMAKE_CURRENT_SOURCE_DIR}/window/constrainment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/window/constrainment/src
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
    ${libdir}
)

add_library (compiz_core SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/global.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/region.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/actions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/screen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/option.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/match.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/output.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/size.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/windowgeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/icon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modifierhandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/propertywriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eventsource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stackdebugger.cpp
    
    ${_bcop_sources}
)

add_executable (compiz
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# workaround for build race
add_dependencies (compiz core-xml-file)

get_property (CORE_MOD_LIBRARIES
	      GLOBAL
	      PROPERTY CORE_MOD_LIBRARIES)

target_link_libraries (
    compiz_core
    
    ${COMPIZ_LIBRARIES} 
    
    m 
    pthread 
    dl
    
    compiz_string
    compiz_timer
    compiz_logmessage
    compiz_pluginclasshandler
    compiz_point
    compiz_rect
    compiz_window_geometry
    compiz_window_geometry_saver
    compiz_window_extents
    compiz_window_constrainment
#    ${CORE_MOD_LIBRARIES}
)

target_link_libraries (
    compiz
    compiz_core
)

install (
    TARGETS compiz_core
  	LIBRARY DESTINATION ${COMPIZ_DESTDIR}${libdir}
)

install (
    TARGETS compiz
    RUNTIME DESTINATION ${COMPIZ_DESTDIR}${exec_prefix}
)

add_subdirectory(tests)

enable_coverage_report( TARGETS compiz )
