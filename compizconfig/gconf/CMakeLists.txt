project (compizconfig-backend-gconf)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

compizconfig_backend (gconf PKGDEPS glib-2.0 gconf-2.0)
