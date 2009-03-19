find_package (Compiz REQUIRED)
include (FindOpenGL)
include (CompizPlugin)

if (OPENGL_GLU_FOUND)
    compiz_plugin (expo PLUGINDEPS composite opengl LIBRARIES ${OPENGL_glu_LIBRARY} INCDIRS ${OPENGL_INCLUDE_DIR})
endif (OPENGL_GLU_FOUND)
