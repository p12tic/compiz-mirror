find_package (Compiz REQUIRED)

include (CompizPlugin)

add_definitions (
    -DDATADIR=\\\"${compiz_datadir}\\\"
)

compiz_plugin (colorfilter PLUGINDEPS composite opengl)
