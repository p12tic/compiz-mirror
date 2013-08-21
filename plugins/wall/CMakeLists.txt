find_package (Compiz REQUIRED)

include (CompizPlugin)
include (CompizDefaults)

compiz_plugin (wall PLUGINDEPS composite opengl mousepoll PKGDEPS cairo cairo-xlib-xrender LIBRARIES dl)
