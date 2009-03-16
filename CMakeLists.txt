find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (notification PKGDEPS libnotify)
