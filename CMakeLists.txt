project (compizconfig-backend-gconf)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

compizconfig_backend (gconf PKGDEPS glib-2.0 gconf-2.0)

# compizconfig-gconf package version number
# An odd micro number indicates in-progress development.
# An even micro number indicates a released version.
set (COMPIZCONFIG_GCONF_VERSION_MAJOR 0)
set (COMPIZCONFIG_GCONF_VERSION_MINOR 9)
set (COMPIZCONFIG_GCONF_VERSION_MICRO 2)
set (COMPIZCONFIG_GCONF_VERSION_MACRO 1)
set (VERSION ${COMPIZCONFIG_GCONF_VERSION_MAJOR}.${COMPIZCONFIG_GCONF_VERSION_MINOR}.${COMPIZCONFIG_GCONF_VERSION_MICRO}.${COMPIZCONFIG_GCONF_VERSION_MACRO})

compiz_print_configure_header ("CompizConfig GConf Storage Backend")
compiz_print_configure_footer ()

compiz_add_uninstall ()
compiz_package_generation ("CompizConfig GConf Storage Backend")
compiz_add_git_dist ()
