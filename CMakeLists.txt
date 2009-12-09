find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (group PLUGINDEPS composite opengl text mousepoll PKGDEPS cairo cairo-xlib-xrender)
