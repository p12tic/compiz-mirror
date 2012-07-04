find_package (Compiz REQUIRED)

include (CompizPlugin)

set (INTERNAL_LIBRARIES
     compiz_opengl_buffer_blit)

add_subdirectory (src/bufferblit)

if (USE_GLES)
    compiz_plugin(opengl PLUGINDEPS composite CFLAGSADD "-DUSE_GLES -std=c++0x" LIBRARIES ${OPENGLES2_LIBRARIES} ${INTERNAL_LIBRARIES} INCDIRS ${OPENGLES2_INCLUDE_DIR})
else (USE_GLES)
    find_package (OpenGL)
    if (OPENGL_FOUND)
	compiz_plugin(opengl PLUGINDEPS composite CFLAGSADD -std=c++0x LIBRARIES ${OPENGL_gl_LIBRARY} ${INTERNAL_LIBRARIES} INCDIRS ${OPENGL_INCLUDE_DIR})
    endif (OPENGL_FOUND)
endif (USE_GLES)

