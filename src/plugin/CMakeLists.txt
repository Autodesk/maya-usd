set(SRC
    plugin.cpp
    renderOverride.cpp
    shadingModeExporter.cpp
    usdPreviewSurface.cpp
    viewCommand.cpp)

add_library(mtoh SHARED ${SRC})
target_link_libraries(mtoh PRIVATE hdmaya usdShade)
target_include_directories(mtoh PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_target_properties(mtoh PROPERTIES PREFIX "")
set_target_properties(mtoh PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
set_target_properties(mtoh PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
MAYA_SET_PLUGIN_PROPERTIES(mtoh)

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # gcc will NOT set this automatically
    target_compile_definitions(mtoh PRIVATE "LINUX")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(mtoh PRIVATE "_WIN32")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Not sure if osx sets this automatically, but won't hurt to redefine
    target_compile_definitions(mtoh PRIVATE "OSMac_")
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

if (LUMA_USD_BUILD)
    target_compile_definitions(mtoh PRIVATE LUMA_USD_BUILD)
endif ()

if (USD_HDST_SHADOWS_BUILD)
    target_compile_definitions(mtoh PRIVATE USD_HDST_SHADOWS_BUILD)
endif ()

if (USD_HDST_UDIM_BUILD)
    target_compile_definitions(mtoh PRIVATE USD_HDST_UDIM_BUILD)
endif ()

install(
    TARGETS mtoh
    DESTINATION plug-ins)
