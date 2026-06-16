cmake_minimum_required (VERSION 3.20)

add_definitions(-DBOOST_ALL_NO_LIB)

project ("UsdLayerEditor")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
add_subdirectory(lib)
add_subdirectory(python)
