find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

compizconfig_backend (gconf glib-2.0 gconf-2.0)
