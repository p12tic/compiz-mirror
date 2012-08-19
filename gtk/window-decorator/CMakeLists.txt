if (USE_GTK)

    if (COMPIZ_BUILD_WITH_RPATH)
	set (CMAKE_INSTALL_RPATH ${libdir})
    endif (COMPIZ_BUILD_WITH_RPATH)

    include_directories (
	${compiz_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/gtk
	${GTK_WINDOW_DECORATOR_INCLUDE_DIRS}
	${METACITY_INCLUDE_DIRS}
	${GCONF_INCLUDE_DIRS}
	${DBUS_GLIB_INCLUDE_DIRS}
    )

    add_definitions (
	-DHAVE_CONFIG_H
	-DALL_LINGUAS=\"${ALL_LINGUAS}\"
	-DLOCALEDIR=\\\"${datadir}/locale\\\"
    )

    link_directories (
	${COMPIZ_LINK_DIRS}
    )

    if (USE_GCONF)
        set (gwd_schema ${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas)
        compiz_translate_xml (
	    ${CMAKE_CURRENT_SOURCE_DIR}/gwd.schemas.in
	    ${gwd_schema}
	)
	compiz_install_gconf_schema (${CMAKE_CURRENT_BINARY_DIR}/gwd.schemas ${SCHEMADIR})
    endif (USE_GCONF)

    add_library (gtk_window_decorator_settings_interface STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-interface.c)

    add_library (gtk_window_decorator_settings STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings.c)

    add_library (gtk_window_decorator_settings_storage_interface STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-storage-interface.c)

    add_library (gtk_window_decorator_settings_storage_gconf STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-storage-gconf.c)

    add_library (gtk_window_decorator_settings_storage_gsettings STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-storage-gsettings.c)

    add_library (gtk_window_decorator_settings_storage_xproperty_interface STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-xproperty-interface.c)

    add_library (gtk_window_decorator_settings_storage_xproperty STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-xproperty-storage.c)

    add_library (gtk_window_decorator_settings_writable_interface STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/gwd-settings-writable-interface.c)

    set (GTK_WINDOW_DECORATOR_ADDITIONAL_LIBRARIES
	 gtk_window_decorator_settings_interface
	 gtk_window_decorator_settings
	 gtk_window_decorator_settings_storage_interface
	 gtk_window_decorator_settings_storage_xproperty_interface
	 gtk_window_decorator_settings_storage_xproperty)

    if (USE_GCONF)
	list (APPEND GTK_WINDOW_DECORATOR_ADDITIONAL_LIBRARIES
	      gtk_window_decorator_settings_storage_gconf)
    endif (USE_GCONF)

    if (USE_GSETTINGS)
	list (APPEND GTK_WINDOW_DECORATOR_ADDITIONAL_LIBRARIES
	      gtk_window_decorator_settings_storage_gsettings)
    endif (USE_GSETTINGS)

    add_executable (gtk-window-decorator
	gtk-window-decorator.c
	blurprops.c
	decorprops.c
	cairo.c
	frames.c
	gdk.c
	switcher.c
	metacity.c
	events.c
	forcequit.c
	tooltip.c
	actionmenu.c
	settings.c
	util.c
	style.c
	wnck.c
	decorator.c
	${gwd_schema}
    )

    add_dependencies (gtk-window-decorator decoration)

    if (USE_METACITY)
	set (metacitylibs ${METACITY_LIBRARIES})
    endif (USE_METACITY)


    target_link_libraries (
	gtk-window-decorator
	decoration
	${GTK_WINDOW_DECORATOR_LIBRARIES}
	${GCONF_LIBRARIES}
	${DBUS_GLIB_LIBRARIES}
	${metacitylibs}
	${GTK_WINDOW_DECORATOR_ADDITIONAL_LIBRARIES}
    )

    install (
	TARGETS gtk-window-decorator
	DESTINATION ${exec_prefix}
    )

    if (COMPIZ_BUILD_TESTING)
	add_subdirectory (tests)
    endif (COMPIZ_BUILD_TESTING)


endif (USE_GTK)



