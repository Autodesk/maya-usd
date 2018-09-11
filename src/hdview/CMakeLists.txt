set(HDVIEW_SRC
    plugin.cpp
    renderOverride.cpp
    usdPreviewSurface.cpp
    viewCommand.cpp)

add_library(hdview SHARED ${HDVIEW_SRC})
target_link_libraries(hdview PRIVATE hdmaya)
target_include_directories(hdview PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
set_target_properties(hdview PROPERTIES PREFIX "")
set_target_properties(hdview PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
set_target_properties(hdview PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # gcc will NOT set this automatically
    target_compile_definitions(hdview PRIVATE "LINUX")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(hdview PRIVATE "_WIN32")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Not sure if osx sets this automatically, but won't hurt to redefine
    target_compile_definitions(hdview PRIVATE "OSMac_")
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

if (LUMA_USD_BUILD)
    target_compile_definitions(hdview PRIVATE LUMA_USD_BUILD)
endif ()

if (USD_HDST_SHADOWS_BUILD)
    target_compile_definitions(hdview PRIVATE USD_HDST_SHADOWS_BUILD)
endif ()

if (USD_HDST_UDIM_BUILD)
    target_compile_definitions(hdview PRIVATE USD_HDST_UDIM_BUILD)
endif ()


install(
    TARGETS hdview
    DESTINATION plug-ins)
