project (compizconfig-backend-gconf)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

compizconfig_backend (gconf PKGDEPS glib-2.0 gconf-2.0)

compiz_print_configure_header ("CompizConfig GConf Storage Backend")
compiz_print_configure_footer ()
compiz_package_generation ("CompizConfig GConf Storage Backend")
