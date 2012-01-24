find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (composite)

if (COMPIZ_BUILD_TESTING)
add_subdirectory (tests)
endif (COMPIZ_BUILD_TESTING)
