pkg_check_modules (
  GLIBMM
  REQUIRED
  glibmm-2.4 glib-2.0 compiz
)

INCLUDE_DIRECTORIES (  
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src

  ${CMAKE_SOURCE_DIR}/include
    
  ${Boost_INCLUDE_DIRS}
  
  ${GLIBMM_INCLUDE_DIRS}
)

LINK_DIRECTORIES (${GLIBMM_LIBRARY_DIRS}) 

SET ( 
  PUBLIC_HEADERS 
)

SET ( 
  PRIVATE_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/offset-movement.h
)

SET( 
  SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/offset-movement.cpp
)

ADD_LIBRARY( 
  compiz_wall_offset_movement STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

if (COMPIZ_BUILD_TESTING)
ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
endif (COMPIZ_BUILD_TESTING)

SET_TARGET_PROPERTIES(
  compiz_wall_offset_movement PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

TARGET_LINK_LIBRARIES(
  compiz_wall_offset_movement

  compiz_core
  ${GLIBMM_LIBRARIES}
)
