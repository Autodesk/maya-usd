list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(AL/usd/schemas)

target_include_directories(${SCHEMAS_PACKAGE}
	PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)
