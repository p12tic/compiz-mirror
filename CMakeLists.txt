project (compizconfig-backend-gconf)

find_package (CompizConfig REQUIRED)

include (LibCompizConfigCommon)

# Generate GConf Schemas for existing installed plugins

set (GENERATE_SCHEMAS_FOR_EXISTING_PLUGINS 1 CACHE BOOL "Generate GConf schemas for existing plugins")

if (GENERATE_SCHEMAS_FOR_EXISTING_PLUGINS)
    pkg_check_modules (GCONF gconf-2.0)

    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)
    set (COMPIZ_GCONF_SCHEMAS_XSLT "${CMAKE_SOURCE_DIR}/xslt/compiz_gconf_schemas.xslt")

    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
        set (SCHEMADIR "${CMAKE_INSTALL_PREFIX}/share/gconf/schemas")
    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
        set (SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

    if (NOT GCONF_FOUND OR NOT GCONFTOOL_EXECUTABLE)
	set (GENERATE_SCHEMAS_FOR_EXISTING_PLUGINS 0)
    else ()
	option (
	    COMPIZ_DISABLE_SCHEMAS_INSTALL
	    "Disables gconf schema installation with gconftool"
	    OFF
	)

	set (
	    COMPIZ_INSTALL_GCONF_SCHEMA_DIR ${COMPIZ_INSTALL_GCONF_SCHEMA_DIR} CACHE PATH
	    "Installation path of the gconf schema file"
	)

	function (compiz_install_gconf_schema _src _dst _name)
	    find_program (GCONFTOOL_EXECUTABLE gconftool-2)
	    mark_as_advanced (FORCE GCONFTOOL_EXECUTABLE)

	    if (GCONFTOOL_EXECUTABLE AND NOT COMPIZ_DISABLE_SCHEMAS_INSTALL)
		install (CODE "
			if (\"\$ENV{USER}\" STREQUAL \"root\")
			    exec_program (${GCONFTOOL_EXECUTABLE}
				ARGS \"--get-default-source\"
				OUTPUT_VARIABLE ENV{GCONF_CONFIG_SOURCE})
			    exec_program (${GCONFTOOL_EXECUTABLE}
				ARGS \"--makefile-install-rule ${_src} > /dev/null\")
			else (\"\$ENV{USER}\" STREQUAL \"root\")
			    exec_program (${GCONFTOOL_EXECUTABLE}
				ARGS \"--install-schema-file=${_src} > /dev/null\")
			endif (\"\$ENV{USER}\" STREQUAL \"root\")
			")
	    endif ()
	    install (
		FILES "${_src}"
		DESTINATION "${COMPIZ_DESTDIR}${_dst}"
		RENAME "${_name}"
	    )
	endfunction ()

	# generate gconf schema
	function (compiz_gconf_schema _src _dst _inst _name)
	    find_program (XSLTPROC_EXECUTABLE xsltproc)
	    mark_as_advanced (FORCE XSLTPROC_EXECUTABLE)

	    if (XSLTPROC_EXECUTABLE)
		add_custom_command (
		    OUTPUT ${_dst}
		    COMMAND ${XSLTPROC_EXECUTABLE}
			    -o ${_dst}
			    ${COMPIZ_GCONF_SCHEMAS_XSLT}
			    ${_src}
		    DEPENDS ${_src}
		)
		compiz_install_gconf_schema (${_dst} ${_inst} ${_name})
	    endif ()
	endfunction ()

	# generate schemas for existing plugins in compiz prefix

	file (GLOB _compiz_schema_files "${COMPIZ_PREFIX}/share/compiz/*.xml")
	foreach (_file ${_compiz_schema_files})
	    string (LENGTH "${COMPIZ_PREFIX}/share/compiz/" _dir_len)
	    string (LENGTH ${_file} _path_len)

	    math (EXPR _file_len "${_path_len} - ${_dir_len}")
	    math (EXPR _plugin_name_len "${_file_len} - 4")

	    string (SUBSTRING ${_file} ${_dir_len} ${_file_len} _plugin_name)
	    string (SUBSTRING ${_plugin_name} 0 ${_plugin_name_len} _plugin_name)
	    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "${COMPIZ_PREFIX}/share/gconf/schemas")
	    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	    compiz_gconf_schema (${_file} "${CMAKE_BINARY_DIR}/generated/compiz-${_plugin_name}.schemas"
				 ${PLUGIN_SCHEMADIR} compiz-${_plugin_name}.schemas)
	    list (APPEND _gconf_files_add "${CMAKE_BINARY_DIR}/generated/compiz-${_plugin_name}.schemas")
	endforeach ()

	# generate schemas for existing plugins in package prefix

	file (GLOB _compiz_schema_files "${datadir}/compiz/*.xml")
	foreach (_file ${_compiz_schema_files})
	    string (LENGTH "${datadir}/compiz/" _dir_len)
	    string (LENGTH ${_file} _path_len)

	    math (EXPR _file_len "${_path_len} - ${_dir_len}")
	    math (EXPR _plugin_name_len "${_file_len} - 4")

	    string (SUBSTRING ${_file} ${_dir_len} ${_file_len} _plugin_name)
	    string (SUBSTRING ${_plugin_name} 0 ${_plugin_name_len} _plugin_name)

	    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "${datadir}/gconf/schemas")
	    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

	    compiz_gconf_schema (${_file} "${CMAKE_BINARY_DIR}/generated/package-${_plugin_name}.schemas"
				 ${PLUGIN_SCHEMADIR} compiz-${_plugin_name}.schemas)
	    list (APPEND _gconf_files_add "${CMAKE_BINARY_DIR}/generated/package-${_plugin_name}.schemas")
	endforeach ()

	# generate schemas for existing plugins in local prefix

	file (GLOB _compiz_schema_files "$ENV{HOME}/.compiz-1/metadata/*.xml")
	foreach (_file ${_compiz_schema_files})
	    string (LENGTH "$ENV{HOME}/.compiz-1/metadata/" _dir_len)
	    string (LENGTH ${_file} _path_len)

	    math (EXPR _file_len "${_path_len} - ${_dir_len}")
	    math (EXPR _plugin_name_len "${_file_len} - 4")

	    string (SUBSTRING ${_file} ${_dir_len} ${_file_len} _plugin_name)
	    string (SUBSTRING ${_plugin_name} 0 ${_plugin_name_len} _plugin_name)

	    if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "$ENV{HOME}/.gconf/schemas")
	    else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
		set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	    endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

	    compiz_gconf_schema (${_file} "${CMAKE_BINARY_DIR}/generated/local-${_plugin_name}.schemas"
				 ${PLUGIN_SCHEMADIR} compiz-${_plugin_name}.schemas)
	    list (APPEND _gconf_files_add "${CMAKE_BINARY_DIR}/generated/local-${_plugin_name}.schemas")
	endforeach ()

    endif ()
endif ()

compizconfig_backend (gconf PKGDEPS glib-2.0 gconf-2.0 FILESADD ${_gconf_files_add})

add_subdirectory (xslt)
add_subdirectory (cmake)

# compizconfig-gconf package version number
# An odd micro number indicates in-progress development.
# An even micro number indicates a released version.
set (COMPIZCONFIG_GCONF_VERSION_MAJOR 0)
set (COMPIZCONFIG_GCONF_VERSION_MINOR 9)
set (COMPIZCONFIG_GCONF_VERSION_MICRO 2)
set (COMPIZCONFIG_GCONF_VERSION_MACRO 1)
set (VERSION ${COMPIZCONFIG_GCONF_VERSION_MAJOR}.${COMPIZCONFIG_GCONF_VERSION_MINOR}.${COMPIZCONFIG_GCONF_VERSION_MICRO}.${COMPIZCONFIG_GCONF_VERSION_MACRO})

compiz_print_configure_header ("CompizConfig GConf Storage Backend")
compiz_color_message ("\n${_escape}[4mOptional features:${_escape}[0m\n")

compiz_print_result_message ("generate schemas for existing plugins" GENERATE_SCHEMAS_FOR_EXISTING_PLUGINS)
compiz_print_configure_footer ()

compiz_add_uninstall ()
compiz_package_generation ("CompizConfig GConf Storage Backend")
compiz_add_git_dist ()
