cmake_minimum_required (VERSION 2.6)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message (SEND_ERROR "Building in the source directory is not supported.")
    message (FATAL_ERROR "Please remove the created \"CMakeCache.txt\" file, the \"CMakeFiles\" directory and create a build directory and call \"${CMAKE_COMMAND} <path to the sources>\".")
endif ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

#### CTest
include (CTest)

#### policies

cmake_policy (SET CMP0000 OLD)
cmake_policy (SET CMP0002 OLD)
cmake_policy (SET CMP0003 NEW)
cmake_policy (SET CMP0005 OLD)
cmake_policy (SET CMP0011 OLD)

set (CMAKE_SKIP_RPATH FALSE)

option (COMPIZ_BUILD_WITH_RPATH "Leave as ON unless building packages" ON)
option (COMPIZ_RUN_LDCONFIG "Leave OFF unless you need to run ldconfig after install")
option (COMPIZ_PACKAGING_ENABLED "Enable to manually set prefix, exec_prefix, libdir, includedir, datadir" OFF)
set (COMPIZ_DESTDIR "${DESTDIR}" CACHE STRING "Leave blank unless building packages")

if (NOT COMPIZ_DESTDIR)
    set (COMPIZ_DESTDIR $ENV{DESTDIR})
endif ()

set (COMPIZ_DATADIR ${CMAKE_INSTALL_PREFIX}/share)
set (COMPIZ_METADATADIR ${CMAKE_INSTALL_PREFIX}/share/compiz)
set (COMPIZ_IMAGEDIR ${CMAKE_INSTALL_PREFIX}/share/compiz/images)
set (COMPIZ_PLUGINDIR ${libdir}/compiz)
set (COMPIZ_SYSCONFDIR ${sysconfdir})

set (
    VERSION ${VERSION} CACHE STRING
    "Package version that is added to a plugin pkg-version file"
)

set (
    COMPIZ_I18N_DIR ${COMPIZ_I18N_DIR} CACHE PATH "Translation file directory"
)

option (COMPIZ_SIGN_WARNINGS "Should compiz use -Wsign-conversion during compilation." OFF)

if (COMPIZ_SIGN_WARNINGS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wsign-conversion")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wsign-conversion")
else ()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif ()

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
    set(IS_GIT_REPO 1)
else (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
    set(IS_GIT_REPO 0)
endif (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

if (EXISTS ${CMAKE_SOURCE_DIR}/.gitmodules)
    set(IS_GIT_SUBMODULES_REPO 1)
else (EXISTS ${CMAKE_SOURCE_DIR}/.gitmodules)
    set(IS_GIT_SUBMODULES_REPO 0)
endif (EXISTS ${CMAKE_SOURCE_DIR}/.gitmodules)

if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.bzr)
    set(IS_BZR_REPO 1)
elseif (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.bzr)
    set(IS_BZR_REPO 0)
endif (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.bzr)

function (compiz_ensure_linkage)
    find_program (LDCONFIG_EXECUTABLE ldconfig)
    mark_as_advanced (FORCE LDCONFIG_EXECUTABLE)

    if (LDCONFIG_EXECUTABLE AND ${COMPIZ_RUN_LDCONFIG})

    install (
        CODE "message (\"Running \" ${LDCONFIG_EXECUTABLE} \" \" ${CMAKE_INSTALL_PREFIX} \"/lib\")
	      exec_program (${LDCONFIG_EXECUTABLE} ARGS \"-v\" ${CMAKE_INSTALL_PREFIX}/lib)"
        )

    endif (LDCONFIG_EXECUTABLE AND ${COMPIZ_RUN_LDCONFIG})
endfunction ()

macro (compiz_add_git_dist)

	# Try to use the git and bzr inbuilt functions for generating
	# archives first, otherwise do it manually
	if (${IS_GIT_REPO})

		if (${IS_GIT_SUBMODULES_REPO})
			find_program (GIT_ARCHIVE_ALL git-archive-all.sh)

			if (NOT (${GIT_ARCHIVE_ALL} STREQUAL "GIT_ARCHIVE_ALL-NOTFOUND"))
				add_custom_target (dist ${GIT_ARCHIVE_ALL} --prefix ${CMAKE_PROJECT_NAME}-${VERSION}/ ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar
						   COMMAND bzip2 ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar
						   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						   COMMENT "Creating bz2 archive")
			else (NOT (${GIT_ARCHIVE_ALL} STREQUAL "GIT_ARCHIVE_ALL-NOTFOUND"))
				add_custom_target (dist
						   COMMAND echo "[WARNING]: git-archive-all.sh is needed to make releases of git submodules, get it from https://github.com/meitar/git-archive-all.sh.git and install it into your PATH"
						   COMMENT "Creating bz2 archive"
						   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
				message ("[WARNING]: git-archive-all.sh is needed to make releases of git submodules, get it from https://github.com/meitar/git-archive-all.sh.git and install it into your PATH")
			endif (NOT (${GIT_ARCHIVE_ALL} STREQUAL "GIT_ARCHIVE_ALL-NOTFOUND"))
		else (${IS_GIT_SUBMODULES_REPO})
			add_custom_target (dist git archive --format=tar --prefix ${CMAKE_PROJECT_NAME}-${VERSION}/ -o ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar HEAD
						   COMMAND bzip2 ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar
						   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						   COMMENT "Creating bz2 archive")
		endif (${IS_GIT_SUBMODULES_REPO})
	else (${IS_GIT_REPO})
		if (${IS_BZR_REPO})
			add_custom_target (dist
					   COMMAND bzr export --root=${CMAKE_PROJECT_NAME}-${VERSION} ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2
					   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		else (${IS_BZR_REPO})
			add_custom_target (dist)
			#add_custom_target (dist 
			#		   COMMAND tar -cvf ${CMAKE_SOURCE_DIR} | bzip2 > ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2
			#		   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../)
		endif (${IS_BZR_REPO})
	endif (${IS_GIT_REPO})

endmacro ()

macro (compiz_add_distcheck)
	add_custom_target (distcheck 
			   COMMAND mkdir -p ${CMAKE_BINARY_DIR}/dist-build
			   && cp ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2 ${CMAKE_BINARY_DIR}/dist-build
			   && cd ${CMAKE_BINARY_DIR}/dist-build
			   && tar xvf ${CMAKE_BINARY_DIR}/dist-build/${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2
			   && mkdir -p ${CMAKE_BINARY_DIR}/dist-build/${CMAKE_PROJECT_NAME}-${VERSION}/build
			   && cd ${CMAKE_BINARY_DIR}/dist-build/${CMAKE_PROJECT_NAME}-${VERSION}/build
			   && cmake -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dist-build/buildroot -DCOMPIZ_PLUGIN_INSTALL_TYPE='package' .. -DCMAKE_MODULE_PATH=/usr/share/cmake -DCOMPIZ_DISABLE_PLUGIN_KDE=ON
			   && make -j4
			   && make test
			   && make -j4 install
			   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	add_dependencies (distcheck dist)
endmacro ()

macro (compiz_add_release_signoff)

	set (AUTO_VERSION_UPDATE "" CACHE STRING "Automatically update VERSION to this number")

	if (AUTO_VERSION_UPDATE)
		message ("-- Next version will be " ${AUTO_VERSION_UPDATE})
	endif (AUTO_VERSION_UPDATE)

	add_custom_target (release-signoff)

	add_custom_target (release-update-working-tree
			   COMMAND cp NEWS ${CMAKE_SOURCE_DIR} &&
				   cp AUTHORS ${CMAKE_SOURCE_DIR} &&
				   cp ChangeLog ${CMAKE_SOURCE_DIR}) 

	if (${IS_GIT_REPO})
		add_custom_target (release-commits
				   COMMAND git commit -a -m "Update NEWS for ${VERSION}"
				   COMMENT "Release Commit"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_custom_target (release-tags
				   COMMAND git tag -u $ENV{RELEASE_KEY} compiz-${VERSION} HEAD -m "Compiz ${VERSION} Release"
				   COMMENT "Release Tags"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_custom_target (release-branch
				   COMMAND git checkout -b compiz-${VERSION}-series && 
				   touch RELEASED && 
				   git add RELEASED &&
				   cat VERSION > RELEASED &&
				   git commit -a -m "Add RELEASED file"
				   COMMENT "Release Branch"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_custom_target (release-update-dist
				   COMMAND git checkout master &&
				   rm ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2 &&
				   make dist
				   COMMENT "Updating bz2 archive"
				   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

		if (AUTO_VERSION_UPDATE)
			add_custom_target (release-version-bump
					   COMMAND git checkout master &&
					   echo "${AUTO_VERSION_UPDATE}" > VERSION &&
					   git add VERSION &&
					   git commit VERSION -m "Bump VERSION"
					   COMMENT "Bumping VERSION"
					   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		else (AUTO_VERSION_UPDATE)
			add_custom_target (release-version-bump
					   COMMAND git checkout master &&
					   $ENV{EDITOR} VERSION &&
					   git add VERSION &&		
					   git commit VERSION -m "Bump VERSION"
					   COMMENT "Bumping VERSION"
					   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		endif (AUTO_VERSION_UPDATE)

	else (${IS_GIT_REPO})
	    add_custom_target (release-commits)
	    add_custom_target (release-tags)
	    add_custom_target (release-branch)
	    add_custom_target (release-update-dist)
	    add_custom_target (release-version-bump)

	endif (${IS_GIT_REPO})

	add_custom_target (release-sign-tarballs
		   COMMAND gpg --armor --sign --detach-sig ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2
	   COMMENT "Signing tarball"
	   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	add_custom_target (release-sha1-tarballs
		   COMMAND sha1sum ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2 > ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2.sha1
		   COMMENT "SHA1Summing tarball"
		   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	add_custom_target (release-sign-sha1-tarballs
		   COMMAND gpg --armor --sign --detach-sig ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2.sha1
		   COMMENT "Signing SHA1Sum checksum"
		   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	add_dependencies (release-commits release-update-working-tree)
	add_dependencies (release-tags release-commits)
	add_dependencies (release-branch release-tags)
	add_dependencies (release-update-dist release-branch)
	add_dependencies (release-version-bump release-update-dist)
	add_dependencies (release-sign-tarballs release-version-bump)
	add_dependencies (release-sha1-tarballs release-sign-tarballs)
	add_dependencies (release-sign-sha1-tarballs release-sha1-tarballs)

	# This means that releasing needs to be done from a git repo for now
	# But that's fine
	add_dependencies (release-signoff release-sign-sha1-tarballs)

	# Actually pushes the release
	if (${IS_GIT_REPO})
		add_custom_target (push-master
				   COMMAND git push origin master
				   COMMENT "Pushing to master"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_custom_target (push-release-branch
				   COMMAND git push origin compiz-${VERISON}-series
				   COMMENT "Pushing to compiz-${VERISON}-series"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_custom_target (push-tag
				   COMMAND git push origin compiz-${VERSION}
				   COMMENT "Pushing tag compiz-${VERSION}"
				   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	else (${IS_GIT_REPO})
		add_custom_target (push-master)
		add_custom_target (push-release-branch)
		add_custom_target (push-tag)
	endif (${IS_GIT_REPO})

	add_custom_target (release-push)

	add_dependencies (release-push push-release-branch)
	add_dependencies (push-release-branch push-tag)
	add_dependencies (push-tag push-master)

	# Push the tarball to releases.compiz.org
	set (COMPIZ_RELEASE_UPLOAD_HOST 	releases.compiz.org)
	set (COMPIZ_RELEASE_UPLOAD_BASE 	/home/releases)
	set (COMPIZ_RELEASE_UPLOAD_DIR_VERSION	${COMPIZ_RELEASE_UPLOAD_BASE}/${VERSION}/)
	set (COMPIZ_RELEASE_UPLOAD_DIR_COMPONENT ${COMPIZ_RELEASE_UPLOAD_BASE}/components/${CMAKE_PROJECT_NAME})

	message ("releasing to " ${COMPIZ_RELEASE_UPLOAD_HOST})
	message (" base: " ${COMPIZ_RELEASE_UPLOAD_BASE})
	message (" version: " ${COMPIZ_RELEASE_UPLOAD_BASE}/${VERSION})
	message (" component: ${COMPIZ_RELEASE_UPLOAD_BASE}/components/${CMAKE_PROJECT_NAME}")

	add_custom_target (release-upload-version
			   COMMAND ssh $ENV{USER}@${COMPIZ_RELEASE_UPLOAD_HOST} "mkdir -p ${COMPIZ_RELEASE_UPLOAD_BASE}/${VERSION}" && scp ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2 $ENV{USER}@${COMPIZ_RELEASE_UPLOAD_HOST}:${COMPIZ_RELEASE_UPLOAD_DIR_VERSION}
			   COMMENT "Uploading ${CMAKE_PROJECT_NAME}-${VERSION} to ${VERSION}"
			   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	# Does nothing for now
	add_custom_target (release-upload-component)
	#add_custom_target (release-upload-component
	#		   COMMAND ssh $ENV{USER}@${COMPIZ_RELEASE_UPLOAD_HOST} "mkdir -p ${COMPIZ_RELEASE_UPLOAD_BASE}/${VERSION}" && scp ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2 ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2.asc ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2.sha1 ${CMAKE_PROJECT_NAME}-${VERSION}.tar.bz2.sha1.asc $ENV{USER}@${COMPIZ_RELEASE_UPLOAD_HOST}:${COMPIZ_RELEASE_UPLOAD_BASE_DIR_COMPONENT}
	#		   COMMENT "Uploading ${CMAKE_PROJECT_NAME}-${VERSION} to ${CMAKE_PROJECT_NAME}/${VERSION}"
	#		   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	add_custom_target (release-upload)

	add_dependencies (release-upload-component release-upload-version)
	add_dependencies (release-upload release-upload-component)

endmacro ()

macro (compiz_add_release)

	set (AUTO_NEWS_UPDATE "" CACHE STRING "Value to insert into NEWS file, leave blank to get an editor when running make news-update")

	if (AUTO_NEWS_UPDATE)
		message ("-- Using auto news update: " ${AUTO_NEWS_UPDATE})
	endif (AUTO_NEWS_UPDATE)

	if (${IS_GIT_REPO})
		find_program (GEN_GIT_LOG gen-git-log.sh)
		add_custom_target (authors
				   COMMAND git shortlog -se | cut -c8- > AUTHORS
				   COMMENT "Generating AUTHORS"
				   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		if (NOT (${GEN_GIT_LOG} STREQUAL "GEN_GIT_LOG-NOTFOUND"))
			add_custom_target (changelog
					   COMMAND ${GEN_GIT_LOG} > ChangeLog
					   COMMENT "Generating ChangeLog"
					   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		else (NOT (${GEN_GIT_LOG} STREQUAL "GEN_GIT_LOG-NOTFOUND"))
			add_custom_target (changelog
					   COMMAND echo "[WARNING]: gen-git-log.sh is required to make releases, ensure that it is installed into your PATH"
					   COMMENT "Generating ChangeLog"
					   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			message ("[WARNING]: gen-git-log.sh is required to make releases, ensure that it is installed into your PATH")
		endif (NOT (${GEN_GIT_LOG} STREQUAL "GEN_GIT_LOG-NOTFOUND"))

		if (AUTO_NEWS_UPDATE)
			add_custom_target (news-header echo > ${CMAKE_BINARY_DIR}/NEWS.update
					   COMMAND echo 'Release ${VERSION} ('`date +%Y-%m-%d`' '`git config user.name`' <'`git config user.email`'>)' > ${CMAKE_BINARY_DIR}/NEWS.update && seq -s "=" `cat ${CMAKE_BINARY_DIR}/NEWS.update | wc -c` | sed 's/[0-9]//g' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo '${AUTO_NEWS_UPDATE}' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo >> ${CMAKE_BINARY_DIR}/NEWS.update
					   COMMENT "Generating NEWS Header"
					   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		else (AUTO_NEWS_UPDATE)
			add_custom_target (news-header echo > ${CMAKE_BINARY_DIR}/NEWS.update
					   COMMAND echo 'Release ${VERSION} ('`date +%Y-%m-%d`' '`git config user.name`' <'`git config user.email`'>)' > ${CMAKE_BINARY_DIR}/NEWS.update && seq -s "=" `cat ${CMAKE_BINARY_DIR}/NEWS.update | wc -c` | sed 's/[0-9]//g' >> ${CMAKE_BINARY_DIR}/NEWS.update && $ENV{EDITOR} ${CMAKE_BINARY_DIR}/NEWS.update && echo >> ${CMAKE_BINARY_DIR}/NEWS.update
					   COMMENT "Generating NEWS Header"
					   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		endif (AUTO_NEWS_UPDATE)

	else (${IS_GIT_REPO})
		if (${IS_BZR_REPO})
			add_custom_target (authors
					   COMMAND bzr log --long --levels=0 | grep -e "^\\s*author:" -e "^\\s*committer:" | cut -d ":" -f 2 | sort -u > AUTHORS
					   COMMENT "Generating AUTHORS")
			add_custom_target (changelog
					   COMMAND bzr log --gnu-changelog > ChangeLog
					   COMMENT "Generating ChangeLog")

			if (AUTO_NEWS_UPDATE)
				message ("running echo 'Release ${VERSION} ('`date +%Y-%m-%d`' '`bzr config email`')' > ${CMAKE_BINARY_DIR}/NEWS.update && seq -s "=" `cat ${CMAKE_BINARY_DIR}/NEWS.update | wc -c` | sed 's/[0-9]//g' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo '${AUTO_NEWS_UPDATE}' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo >> ${CMAKE_BINARY_DIR}/NEWS.update")
				add_custom_target (news-header echo > ${CMAKE_BINARY_DIR}/NEWS.update
						   COMMAND echo 'Release ${VERSION} ('`date +%Y-%m-%d`' '`bzr config email`')' > ${CMAKE_BINARY_DIR}/NEWS.update && seq -s "=" `cat ${CMAKE_BINARY_DIR}/NEWS.update | wc -c` | sed 's/[0-9]//g' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo '${AUTO_NEWS_UPDATE}' >> ${CMAKE_BINARY_DIR}/NEWS.update && echo >> ${CMAKE_BINARY_DIR}/NEWS.update
						   COMMENT "Generating NEWS Header"
						   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
			else (AUTO_NEWS_UPDATE)
				add_custom_target (news-header echo > ${CMAKE_BINARY_DIR}/NEWS.update
						   COMMAND echo 'Release ${VERSION} ('`date +%Y-%m-%d`' '`bzr config email`')' > ${CMAKE_BINARY_DIR}/NEWS.update && seq -s "=" `cat ${CMAKE_BINARY_DIR}/NEWS.update | wc -c` | sed 's/[0-9]//g' >> ${CMAKE_BINARY_DIR}/NEWS.update && $ENV{EDITOR} ${CMAKE_BINARY_DIR}/NEWS.update && echo >> ${CMAKE_BINARY_DIR}/NEWS.update
						   COMMENT "Generating NEWS Header"
						   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
			endif (AUTO_NEWS_UPDATE)

		else (${IS_BZR_REPO})
			add_custom_target (authors)
			add_custom_target (changelog)
			add_custom_target (news-header)
		endif (${IS_BZR_REPO})
	endif (${IS_GIT_REPO})

	add_custom_target (news
			   COMMAND touch ${CMAKE_SOURCE_DIR}/NEWS
				   cat ${CMAKE_SOURCE_DIR}/NEWS > NEWS.old &&
				   cat NEWS.old >> ${CMAKE_BINARY_DIR}/NEWS.update &&
				   cat NEWS.update > NEWS
			   WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

	add_dependencies (changelog authors)
	add_dependencies (news-header changelog)
	add_dependencies (news news-header)

	add_custom_target (release-prep)
	add_dependencies (release-prep news)

endmacro (compiz_add_release)

# unsets the given variable
macro (compiz_unset var)
    set (${var} "" CACHE INTERNAL "")
endmacro ()

# sets the given variable
macro (compiz_set var value)
    set (${var} ${value} CACHE INTERNAL "")
endmacro ()


macro (compiz_format_string str length return)
    string (LENGTH "${str}" _str_len)
    math (EXPR _add_chr "${length} - ${_str_len}")
    set (${return} "${str}")
    while (_add_chr GREATER 0)
	set (${return} "${${return}} ")
	math (EXPR _add_chr "${_add_chr} - 1")
    endwhile ()
endmacro ()

string (ASCII 27 _escape)
function (compiz_color_message _str)
    if (CMAKE_COLOR_MAKEFILE)
	message (${_str})
    else ()
	string (REGEX REPLACE "${_escape}.[0123456789;]*m" "" __str ${_str})
	message (${__str})
    endif ()
endfunction ()

function (compiz_configure_file _src _dst)
    foreach (_val ${ARGN})
        set (_${_val}_sav ${${_val}})
        set (${_val} "")
	foreach (_word ${_${_val}_sav})
	    set (${_val} "${${_val}}${_word} ")
	endforeach (_word ${_${_val}_sav})
    endforeach (_val ${ARGN})

    configure_file (${_src} ${_dst} @ONLY)

    foreach (_val ${ARGN})
	set (${_val} ${_${_val}_sav})
        set (_${_val}_sav "")
    endforeach (_val ${ARGN})
endfunction ()

function (compiz_add_plugins_in_folder folder)
    set (COMPIZ_PLUGIN_PACK_BUILD 1)
    file (
        GLOB _plugins_in
        RELATIVE "${folder}"
        "${folder}/*/CMakeLists.txt"
    )

    foreach (_plugin ${_plugins_in})
        get_filename_component (_plugin_dir ${_plugin} PATH)
        add_subdirectory (${folder}/${_plugin_dir})
    endforeach ()
endfunction ()

#### pkg-config handling

include (FindPkgConfig)

function (compiz_pkg_check_modules _var _req)
    if (NOT ${_var})
        pkg_check_modules (${_var} ${_req} ${ARGN})
	if (${_var}_FOUND)
	    set (${_var} 1 CACHE INTERNAL "" FORCE)
	endif ()
	set(__pkg_config_checked_${_var} 0 CACHE INTERNAL "" FORCE)
    endif ()
endfunction ()

#### translations

# translate metadata file
function (compiz_translate_xml _src _dst)
    find_program (INTLTOOL_MERGE_EXECUTABLE intltool-merge)
    mark_as_advanced (FORCE INTLTOOL_MERGE_EXECUTABLE)

    if (INTLTOOL_MERGE_EXECUTABLE
	AND COMPIZ_I18N_DIR
	AND EXISTS ${COMPIZ_I18N_DIR})
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${INTLTOOL_MERGE_EXECUTABLE} -x -u -c
		    ${CMAKE_BINARY_DIR}/.intltool-merge-cache
		    ${COMPIZ_I18N_DIR}
		    ${_src}
		    ${_dst}
	    DEPENDS ${_src}
	)
    else ()
    	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND cat ${_src} |
		    sed -e 's;<_;<;g' -e 's;</_;</;g' > 
		    ${_dst}
	    DEPENDS ${_src}
	)
    endif ()
endfunction ()

function (compiz_translate_desktop_file _src _dst)
    find_program (INTLTOOL_MERGE_EXECUTABLE intltool-merge)
    mark_as_advanced (FORCE INTLTOOL_MERGE_EXECUTABLE)

    if (INTLTOOL_MERGE_EXECUTABLE
	AND COMPIZ_I18N_DIR
	AND EXISTS ${COMPIZ_I18N_DIR})
	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND ${INTLTOOL_MERGE_EXECUTABLE} -d -u -c
		    ${CMAKE_BINARY_DIR}/.intltool-merge-cache
		    ${COMPIZ_I18N_DIR}
		    ${_src}
		    ${_dst}
	    DEPENDS ${_src}
	)
    else ()
    	add_custom_command (
	    OUTPUT ${_dst}
	    COMMAND cat ${_src} |
		    sed -e 's;^_;;g' >
		    ${_dst}
	    DEPENDS ${_src}
	)
    endif ()
endfunction ()

#### optional file install

function (compiz_opt_install_file _src _dst)
    install (CODE
        "message (\"-- Installing: ${_dst}\")
         execute_process (
	    COMMAND ${CMAKE_COMMAND} -E copy_if_different \"${_src}\" \"${COMPIZ_DESTDIR}${_dst}\"
	    RESULT_VARIABLE _result
	    OUTPUT_QUIET ERROR_QUIET
	 )
	 if (_result)
	     message (\"-- Failed to install: ${_dst}\")
	 endif ()
        "
    )
endfunction ()

#### uninstall

macro (compiz_add_uninstall)
   if (NOT _compiz_uninstall_rule_created)
	compiz_set(_compiz_uninstall_rule_created TRUE)

	set (_file "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")

	file (WRITE  ${_file} "if (NOT EXISTS \"${CMAKE_BINARY_DIR}/install_manifest.txt\")\n")
	file (APPEND ${_file} "  message (FATAL_ERROR \"Cannot find install manifest: \\\"${CMAKE_BINARY_DIR}/install_manifest.txt\\\"\")\n")
	file (APPEND ${_file} "endif (NOT EXISTS \"${CMAKE_BINARY_DIR}/install_manifest.txt\")\n\n")
	file (APPEND ${_file} "file (READ \"${CMAKE_BINARY_DIR}/install_manifest.txt\" files)\n")
	file (APPEND ${_file} "string (REGEX REPLACE \"\\n\" \";\" files \"\${files}\")\n")
	file (APPEND ${_file} "foreach (file \${files})\n")
	file (APPEND ${_file} "  message (STATUS \"Uninstalling \\\"\${file}\\\"\")\n")
	file (APPEND ${_file} "  if (EXISTS \"\${file}\")\n")
	file (APPEND ${_file} "    exec_program(\n")
	file (APPEND ${_file} "      \"${CMAKE_COMMAND}\" ARGS \"-E remove \\\"\${file}\\\"\"\n")
	file (APPEND ${_file} "      OUTPUT_VARIABLE rm_out\n")
	file (APPEND ${_file} "      RETURN_VALUE rm_retval\n")
	file (APPEND ${_file} "      )\n")
	file (APPEND ${_file} "    if (\"\${rm_retval}\" STREQUAL 0)\n")
	file (APPEND ${_file} "    else (\"\${rm_retval}\" STREQUAL 0)\n")
	file (APPEND ${_file} "      message (FATAL_ERROR \"Problem when removing \\\"\${file}\\\"\")\n")
	file (APPEND ${_file} "    endif (\"\${rm_retval}\" STREQUAL 0)\n")
	file (APPEND ${_file} "  else (EXISTS \"\${file}\")\n")
	file (APPEND ${_file} "    message (STATUS \"File \\\"\${file}\\\" does not exist.\")\n")
	file (APPEND ${_file} "  endif (EXISTS \"\${file}\")\n")
	file (APPEND ${_file} "endforeach (file)\n")

	add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")

    endif ()
endmacro ()

#posix 2008 scandir check
if (CMAKE_CXX_COMPILER)
	include (CheckCXXSourceCompiles)
	CHECK_CXX_SOURCE_COMPILES (
	  "# include <dirent.h>
	   int func (const char *d, dirent ***list, void *sort)
	   {
	     int n = scandir(d, list, 0, (int(*)(const dirent **, const dirent **))sort);
	     return n;
	   }

	   int main (int, char **)
	   {
	     return 0;
	   }
	  "
	  HAVE_SCANDIR_POSIX)
endif (CMAKE_CXX_COMPILER)

if (HAVE_SCANDIR_POSIX)
  add_definitions (-DHAVE_SCANDIR_POSIX)
endif ()
