link_directories (${COMPIZ_TEST_PCH_LINK_DIRS})

include_directories (${COMPIZ_TEST_PCH_INCLUDE_DIRS}
		     ${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated
		     ${compiz_SOURCE_DIR}/src
		     ${COMPIZ_TEST_PCH_BASE_DIR})

set (OBJECT CompizPCHTestIndexes)
add_definitions (-DOBJECT=${OBJECT})
# hack
add_definitions (-Dprivate=public)
add_definitions (-Dprotected=public)

add_executable (test-pch-indexes
		test-pch-indexes.cpp
		../test-pluginclasshandler.cpp 
	        ${compiz_SOURCE_DIR}/src/valueholder.cpp
	        ${compiz_SOURCE_DIR}/src/pluginclasses.cpp
		${compiz_SOURCE_DIR}/src/string.cpp
		${compiz_SOURCE_DIR}/src/logmessage.cpp)

target_link_libraries (test-pch-indexes
		       ${COMPIZ_TEST_PCH_LIBRARIES})

#add_test (test-pch-construct
#	  ${CMAKE_CURRENT_BINARY_DIR}/test-pch-callbacks)
