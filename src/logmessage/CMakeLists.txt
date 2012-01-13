
INCLUDE_DIRECTORIES(  
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  
  ${compiz_SOURCE_DIR}/include
    
  ${Boost_INCLUDE_DIRS}
)

SET( PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/core/logmessage.h )
SET( PRIVATE_HEADERS )

SET( SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/logmessage.cpp )

ADD_LIBRARY( 
  compiz_logmessage STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )

SET_TARGET_PROPERTIES(
  compiz_logmessage PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

INSTALL(
  TARGETS                                 compiz_logmessage
  RUNTIME             DESTINATION         bin
  LIBRARY             DESTINATION         lib
  ARCHIVE             DESTINATION         lib
  PUBLIC_HEADER       DESTINATION         include/compiz/core
)
