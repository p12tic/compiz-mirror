find_package (Threads REQUIRED)

set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_BINARY_DIR}/__gmock")

include_directories (${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})

set (GTEST_BOTH_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} gtest gtest_main)

if (NOT GTEST_BOTH_LIBRARIES)

    message ("Google Test not found - cannot build tests!")
    set (GTEST_FOUND FALSE)

else (NOT GTEST_BOTH_LIBRARIES)

    set (GTEST_FOUND TRUE)

endif (NOT GTEST_BOTH_LIBRARIES)

set (GMOCK_LIBRARY gmock)
set (GMOCK_MAIN_LIBRARY gmock_main)
add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=0)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

    message ("Google Mock and Google Test not found - cannot build tests!")
    set (GOOGLE_TEST_AND_MOCK_FOUND FALSE)

else (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

    set (GOOGLE_TEST_AND_MOCK_FOUND TRUE)

endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)