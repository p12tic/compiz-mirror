pkg_check_modules(
  GLIBMM 
  REQUIRED
  glibmm-2.4 
)

INCLUDE_DIRECTORIES(  
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  
  ${compiz_SOURCE_DIR}/include
    
  ${Boost_INCLUDE_DIRS}
  
  ${GLIBMM_INCLUDE_DIRS}
)

LINK_DIRECTORIES( ${GLIBMM_LIBRARY_DIRS} ) 

SET( 
  PUBLIC_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/timer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/core/timeouthandler.h
)

SET( 
  PRIVATE_HEADERS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/privatetimeouthandler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/privatetimeoutsource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/privatetimer.h

)

SET( 
  SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/timeouthandler.cpp 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/timer.cpp 
)

ADD_LIBRARY( 
  compiz_timer STATIC
  
  ${SRCS}
  
  ${PUBLIC_HEADERS}
  ${PRIVATE_HEADERS}
)

ADD_SUBDIRECTORY( ${CMAKE_CURRENT_SOURCE_DIR}/tests )

SET_TARGET_PROPERTIES(
  compiz_timer PROPERTIES
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

INSTALL(
  TARGETS                                 compiz_timer
  RUNTIME             DESTINATION         bin
  LIBRARY             DESTINATION         lib
  ARCHIVE             DESTINATION         lib
  PUBLIC_HEADER       DESTINATION         include/compiz
)



TARGET_LINK_LIBRARIES( 
  compiz_timer
  
  ${GLIBMM_LIBRARIES}
)
