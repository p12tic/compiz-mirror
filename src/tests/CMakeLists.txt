add_executable (compiz_test_outputdevices
    test_outputdevices.cpp
)

include_directories (
    ..
    ../rect/include
    ../region/include
    ../timer/include
    ../pluginclasshandler/include
    ../window/geometry/include
    ../window/extents/include
    ${COMPIZ_INCLUDE_DIRS}
)

link_directories (..)

target_link_libraries (compiz_test_outputdevices
    -Wl,-start-group
    compiz_size
    compiz_output
    compiz_outputdevices
    compiz_rect
    compiz_region
    compiz_window_geometry
    -Wl,-end-group
    ${GTEST_BOTH_LIBRARIES}
)

compiz_discover_tests(compiz_test_outputdevices COVERAGE compiz_test_outputdevices)

