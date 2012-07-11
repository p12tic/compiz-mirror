# Don't allow subdirectories to add schema recompile rule
set_property (GLOBAL
	      PROPERTY COMPIZ_INHIBIT_ADD_INSTALL_RECOMPILE_RULE
	      FALSE)

if (USE_GSETTINGS)

    compiz_add_install_recompile_gsettings_schemas (${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas)

endif (USE_GSETTINGS)
