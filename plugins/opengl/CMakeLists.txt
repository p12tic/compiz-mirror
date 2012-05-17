find_package (Compiz REQUIRED)

include (CompizPlugin)

if (USE_GLES)
    compiz_plugin(opengl PLUGINDEPS composite CFLAGSADD "-DUSE_GLES -std=c++0x" LIBRARIES ${OPENGLES2_LIBRARIES} INCDIRS ${OPENGLES2_INCLUDE_DIR})
else (USE_GLES)
    find_package (OpenGL)
    if (OPENGL_FOUND)
	compiz_plugin(opengl PLUGINDEPS composite CFLAGSADD -std=c++0x LIBRARIES ${OPENGL_gl_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
    endif (OPENGL_FOUND)
endif (USE_GLES)

