find_package (Compiz REQUIRED)
include (FindOpenGL)
include (CompizPlugin)

if (OPENGL_GLU_FOUND)
    add_subdirectory (src/click_threshold)
    include_directories (src/click_threshold/include)

    compiz_plugin (expo PLUGINDEPS composite opengl LIBRARIES ${OPENGL_glu_LIBRARY} compiz_expo_click_threshold INCDIRS ${OPENGL_INCLUDE_DIR})
endif (OPENGL_GLU_FOUND)

