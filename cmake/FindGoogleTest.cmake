# Find Google Test and Google Mock, either with pkg-config or by
# searching the system install paths. This sets:
#
# GOOGLE_TEST_AND_MOCK_FOUND
# GTEST_INCLUDE_DIR
# GTEST_ROOT_DIR
# GMOCK_LIBRARY
# GMOCK_MAIN_LIBRARY

find_package (GTest)

if (NOT GTEST_FOUND)

    # Check for google test and build it locally
    set (GTEST_ROOT_DIR
 	 "/usr/src/gtest" # Default value, adjustable by user with e.g., ccmake
	 CACHE
	 PATH
	 "Path to Google Test srcs"
	 FORCE)

    find_path (GTEST_INCLUDE_DIR gtest/gtest.h)

    set (GTEST_BOTH_LIBRARIES gtest gtest_main)
    set (GTEST_FOUND TRUE)
    set (GTEST_LOCAL_BUILD_REQUIRED TRUE)

else (NOT GTEST_FOUND)

    set (GTEST_LOCAL_BUILD_REQUIRED FALSE)

endif (NOT GTEST_FOUND)

find_library (GMOCK_LIBRARY gmock)
find_library (GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

    message ("Google Mock and Google Test not found - cannot build tests!")
    set (GOOGLE_TEST_AND_MOCK_FOUND OFF CACHE BOOL "" FORCE)

else (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

    set (GOOGLE_TEST_AND_MOCK_FOUND ON CACHE BOOL "" FORCE)

endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
