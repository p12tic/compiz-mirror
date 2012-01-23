find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (composite)

add_subdirectory (tests)
