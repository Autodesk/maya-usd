project(hd-maya-al)

cmake_minimum_required(VERSION 2.8)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Maya REQUIRED)
find_package(MayaToHydra REQUIRED)

option(HD_MAYA_AL_OVERRIDE_PROXY_SELECTION "Override the AL_usdmaya_ProxyShape's selection to use Maya-to-Hydra's to find picked prims.  This is more efficient, as it doesn't require the ProxyShape to keep it's own GL copy of the scene, just ot use for selection, but requires a version of the AL plugin that has been modified to provide ProxyShape::setFindPickedPrimsFunction and getFindPickedPrimsFunction." OFF)

add_subdirectory(src)
add_subdirectory(python)