option (
    USE_GSETTINGS
    "Generate GSettings schemas"
    ON
)

option (
    COMPIZ_DISABLE_GS_SCHEMAS_INSTALL
    "Disables gsettings schema installation"
    OFF
)

set (
    COMPIZ_INSTALL_GSETTINGS_SCHEMA_DIR ${COMPIZ_INSTALL_GSETTINGS_SCHEMA_DIR} CACHE PATH
    "Installation path of the gsettings schema file"
)

function (compiz_add_install_recompile_gsettings_schemas _schemadir_user _schemadir_root)
	# Recompile GSettings Schemas
	install (CODE "
		 execute_process (COMMAND cmake -DSCHEMADIR_USER=${_schemadir_user} -DSCHEMADIR_ROOT=${_schemadir_root} -P ${compiz_SOURCE_DIR}/cmake/recompile_gsettings_schemas_in_dir_user_env.cmake)
		 ")
endfunction (compiz_add_install_recompile_gsettings_schemas)

function (compiz_install_gsettings_schema _src _dst)
    find_program (PKG_CONFIG_TOOL pkg-config)
    find_program (GLIB_COMPILE_SCHEMAS glib-compile-schemas)
    mark_as_advanced (FORCE PKG_CONFIG_TOOL)

    # find out where schemas need to go if we are installing them systemwide
    execute_process (COMMAND ${PKG_CONFIG_TOOL} glib-2.0 --variable prefix  OUTPUT_VARIABLE GSETTINGS_GLIB_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET (GSETTINGS_GLOBAL_INSTALL_DIR "${GSETTINGS_GLIB_PREFIX}/share/glib-2.0/schemas/")

    if (PKG_CONFIG_TOOL AND
	GLIB_COMPILE_SCHEMAS AND NOT
	COMPIZ_DISABLE_SCHEMAS_INSTALL AND
	USE_GSETTINGS)

	# Install schema file
	install (CODE "
		 execute_process (COMMAND cmake -DFILE=${_src} -DINSTALLDIR_USER=${_dst} -DINSTALLDIR_ROOT=${GSETTINGS_GLOBAL_INSTALL_DIR} -P ${compiz_SOURCE_DIR}/cmake/copy_file_install_user_env.cmake)
		 ")

	compiz_add_install_recompile_gsettings_schemas (${_dst} ${GSETTINGS_GLOBAL_INSTALL_DIR})
    endif (PKG_CONFIG_TOOL AND
	   GLIB_COMPILE_SCHEMAS AND NOT
	   COMPIZ_DISABLE_SCHEMAS_INSTALL AND
	   USE_GSETTINGS)
endfunction ()

function (add_gsettings_schema_to_recompilation_list _target_name_for_schema)

    get_property (GSETTINGS_LOCAL_COMPILE_TARGET_SET
		  GLOBAL
		  PROPERTY GSETTINGS_LOCAL_COMPILE_TARGET_SET
		  SET)

    if (NOT GSETTINGS_LOCAL_COMPILE_TARGET_SET)

	add_custom_command (OUTPUT ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/gschemas.compiled
			   COMMAND ${GLIB_COMPILE_SCHEMAS} --targetdir=${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/
                    	   ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/
			   COMMENT "Recompiling GSettings schemas locally"
	)

	add_custom_target (compiz_gsettings_compile_local ALL
			   DEPENDS ${CMAKE_BINARY_DIR}/generated/glib-2.0/schemas/gschemas.compiled)

	set_property (GLOBAL
		      PROPERTY GSETTINGS_LOCAL_COMPILE_TARGET_SET
		      TRUE)

    endif (NOT GSETTINGS_LOCAL_COMPILE_TARGET_SET)

    add_dependencies (compiz_gsettings_compile_local
		      ${_target_name_for_schema})

endfunction ()

# generate gconf schema
function (compiz_gsettings_schema _name _src _dst _inst)
    find_program (XSLTPROC_EXECUTABLE xsltproc)
    find_program (GLIB_COMPILE_SCHEMAS glib-compile-schemas)
    mark_as_advanced (FORCE XSLTPROC_EXECUTABLE)

    if (XSLTPROC_EXECUTABLE AND GLIB_COMPILE_SCHEMAS AND USE_GSETTINGS)
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${XSLTPROC_EXECUTABLE}
		    -o ${_dst}
		    ${COMPIZ_GSETTINGS_SCHEMAS_XSLT}
		    ${_src}
	    DEPENDS ${_src}
	)

	add_custom_target (${_name}_gsettings_schema
			   DEPENDS ${_dst})

	compiz_install_gsettings_schema (${_dst} ${_inst})
	add_gsettings_schema_to_recompilation_list (${_name}_gsettings_schema)
    endif ()
endfunction ()
