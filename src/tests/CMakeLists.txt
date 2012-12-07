add_executable (compiz_test_outputdevices
    test_outputdevices.cpp
)

include_directories (
    ${COMPIZ_MAIN_SOURCE_DIR}
    ${COMPIZ_MAIN_SOURCE_DIR}/rect/include
    ${COMPIZ_MAIN_SOURCE_DIR}/region/include
    ${COMPIZ_MAIN_SOURCE_DIR}/timer/include
    ${COMPIZ_MAIN_SOURCE_DIR}/pluginclasshandler/include
    ${COMPIZ_MAIN_SOURCE_DIR}/window/geometry/include
    ${COMPIZ_MAIN_SOURCE_DIR}/window/extents/include
    ${COMPIZ_INCLUDE_DIRS}
)

link_directories (${COMPIZ_MAIN_BINARY_DIR})

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

compiz_discover_tests(compiz_test_outputdevices COVERAGE compiz_core)

add_executable (compiz_test_geometryupdatequeue
                test_geometryupdatequeue.cpp)

target_link_libraries (compiz_test_geometryupdatequeue
    compiz_geometryupdatequeue
    ${GTEST_BOTH_LIBRARIES}
    ${GMOCK_MAIN_LIBRARY}
    ${GMOCK_LIBRARY}
)

compiz_discover_tests(compiz_test_geometryupdatequeue COVERAGE compiz_geometryupdatequeue)

add_executable (compiz_test_syncserverwindow
                test_syncserverwindow.cpp)

target_link_libraries (compiz_test_syncserverwindow
                       compiz_syncserverwindow
                       ${GTEST_BOTH_LIBRARIES}
                       ${GMOCK_MAIN_LIBRARY}
                       ${GMOCK_LIBRARY})

compiz_discover_tests(compiz_test_syncserverwindow COVERAGE compiz_syncserverwindow)
