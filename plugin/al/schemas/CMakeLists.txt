list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
add_subdirectory(AL/usd/schemas/maya)
add_subdirectory(AL/usd/schemas/mayatest)

target_include_directories(AL_USDMayaSchemas
	PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)
