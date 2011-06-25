include_directories (${compiz_SOURCE_DIR}/include
		     ${compiz_BINARY_DIR}
		     ${compiz_BINARY_DIR}/generated)

add_executable (test-pluginclasshandler
		test-pluginclasshandler.cpp
	        ${compiz_SOURCE_DIR}/src/valueholder.cpp
	        ${compiz_SOURCE_DIR}/src/pluginclasses.cpp
		${compiz_SOURCE_DIR}/src/string.cpp
		${compiz_SOURCE_DIR}/src/logmessage.cpp)
