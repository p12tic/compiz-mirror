exec_program (${HAVE_PYREX} ${CMAKE_SOURCE_DIR}/src
		  ARGS "compizconfig.pyx")

message ("ccs includes are " ${CCS_PYTHON_LIBRARY_DIRS})
message ("py includes are " ${PYTHON_INCLUDE_DIRS})

include_directories (
	     ${CCS_PYTHON_INCLUDE_DIRS}
	     ${PYTHON_INCLUDE_DIRS}
)

set (CCS_PYTHON_LIBS "")
foreach (_val ${CCS_PYTHON_LDFLAGS})
    set (CCS_PYTHON_LIBS "${CCS_PYTHON_LIBS}${_val} ")
endforeach (_val ${CCS_PYTHON_LDFLAGS})

set (CCS_PYTHON_CFLAGS "")
foreach (_val ${CCS_PYTHON_CFLAGS})
    set (CCS_PYTHON_CFLAGS "${CCS_PYTHON_CFLAGS}${_val} ")
endforeach (_val ${CCS_PYTHON_CFLAGS})

link_directories (
	${CCS_PYTHON_LIBRARY_DIRS}
)

add_library (pycompizconfig SHARED
	     compizconfig.c)

target_link_libraries (
	     pycompizconfig
	     ${CCS_PYTHON_LIBRARIES}
	     ${COMPIZCONFIG_LIBRARIES}
	     ${PYTHON_LIBRARIES}
)

exec_program ("mv" ${CMAKE_BINARY_DIR}/src ARGS "libpycompizconfig.so compizconfig.so")

install (
    FILES ${CMAKE_BINARY_DIR}/src/compizconfig.so
    DESTINATION ${CMAKE_INSTALL_PATH}/lib/python2.6/site-packages
)
