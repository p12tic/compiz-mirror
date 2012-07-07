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
	install (CODE "
		if (\"$ENV{USER}\"\ STREQUAL \"root\")
		    message (\"-- Installing GSettings schemas ${GSETTINGS_GLOBAL_INSTALL_DIR}\"\)
		    file (INSTALL DESTINATION \"${GSETTINGS_GLOBAL_INSTALL_DIR}\"
			  TYPE FILE
			  FILES \"${_src}\")
		    message (\"-- Recompiling GSettings schemas in ${GSETTINGS_GLOBAL_INSTALL_DIR}\"\)
		    execute_process (COMMAND ${GLIB_COMPILE_SCHEMAS} ${GSETTINGS_GLOBAL_INSTALL_DIR})

		else (\"$ENV{USER}\"\ STREQUAL \"root\"\)
		    # It seems like this is only available in CMake > 2.8.5
		    # but hardly anybody has that, so comment out this warning for now
		    # string (FIND $ENV{XDG_DATA_DIRS} \"${_dst}\" XDG_INSTALL_PATH)
		    # if (NOT XDG_INSTALL_PATH)
		    #	message (\"[WARNING]: Installing GSettings schemas to directory that is not in XDG_DATA_DIRS, you need to add ${_dst} to your XDG_DATA_DIRS in order for GSettings schemas to be found!\"\)
		    # endif (NOT XDG_INSTALL_PATH)
		    message (\"-- Installing GSettings schemas to ${_dst}\"\)
		    file (INSTALL DESTINATION \"${_dst}\"
			  TYPE FILE
			  FILES \"${_src}\")
		    message (\"-- Recompiling GSettings schemas in ${_dst}\"\)
		    execute_process (COMMAND ${GLIB_COMPILE_SCHEMAS} ${_dst})
		endif (\"$ENV{USER}\" STREQUAL \"root\"\)
		")
    endif ()
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
