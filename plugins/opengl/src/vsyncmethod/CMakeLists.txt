INCLUDE_DIRECTORIES (
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
    
  ${Boost_INCLUDE_DIRS}
)

LINK_DIRECTORIES (${COMPIZ_LIBRARY_DIRS}) 

SET( 
  SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vsync-method-swap-interval.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/vsync-method-wait-video-sync.cpp
)

ADD_LIBRARY( 
  compiz_opengl_vsync_methods STATIC
  
  ${SRCS}
)

if (COMPIZ_BUILD_TESTING)
ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
endif (COMPIZ_BUILD_TESTING)
