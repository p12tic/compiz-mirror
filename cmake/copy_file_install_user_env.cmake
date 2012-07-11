set (FILE "" CACHE FORCE "File to Install")
set (INSTALLDIR_USER "" CACHE FORCE "Installation dir if user")
set (INSTALLDIR_ROOT "" CACHE FORCE "Installation dir if root")

set (USERNAME $ENV{USER})

if (${USERNAME} STREQUAL "root")
    set (INSTALLDIR ${INSTALLDIR_ROOT})
else (${USERNAME} STREQUAL "root")
    set (INSTALLDIR ${INSTALLDIR_USER})
endif (${USERNAME} STREQUAL "root")

file (INSTALL DESTINATION $ENV{DESTDIR}${INSTALLDIR}
      TYPE FILE
      FILES ${FILE})
