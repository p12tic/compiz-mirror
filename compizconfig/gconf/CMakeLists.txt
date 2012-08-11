project (compizconfig-backend-gconf)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

include_directories (${CMAKE_SOURCE_DIR}/compizconfig/integration/gnome/include)
link_directories (${CMAKE_BINARY_DIR}/compizconfig/integration/gnome)

compizconfig_backend (gconf PKGDEPS glib-2.0 gconf-2.0 LIBRARIES compizconfig_gnome_integration)
