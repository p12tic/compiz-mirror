INCLUDE_DIRECTORIES(
  ${compiz_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  
  ${Boost_INCLUDE_DIRS}
)

SET( PUBLIC_HEADERS )
SET( PRIVATE_HEADERS )

SET( SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/string.cpp )

ADD_LIBRARY( 
  compiz_string STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )

SET_TARGET_PROPERTIES(
  compiz_string PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

INSTALL(
  TARGETS                                 compiz_string
  RUNTIME             DESTINATION         bin
  LIBRARY             DESTINATION         lib
  ARCHIVE             DESTINATION         lib
  PUBLIC_HEADER       DESTINATION         include/compiz
)
