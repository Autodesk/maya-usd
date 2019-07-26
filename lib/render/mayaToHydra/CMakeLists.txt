set(TARGET_NAME mtoh)

set(SRC
    defaultLightDelegate.cpp
    plugin.cpp
    pluginDebugCodes.cpp
    renderGlobals.cpp
    renderOverride.cpp
    shadingModeExporter.cpp
    tokens.cpp
    usdPreviewSurface.cpp
    utils.cpp
    viewCommand.cpp
)

add_library(${TARGET_NAME} SHARED ${SRC})

target_link_libraries(${TARGET_NAME} PRIVATE 
    hdmaya 
    usdShade
    ${MAYA_LIBRARIES}
)

target_include_directories(${TARGET_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${MAYA_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/usd
)

set_target_properties(${TARGET_NAME} PROPERTIES 
    PREFIX ""
    INSTALL_RPATH_USE_LINK_PATH ON
)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../lib")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif ()

maya_set_plugin_properties(${TARGET_NAME})

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # gcc will NOT set this automatically
    target_compile_definitions(${TARGET_NAME} PRIVATE "LINUX")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(${TARGET_NAME} PRIVATE "_WIN32")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Not sure if osx sets this automatically, but won't hurt to redefine
    target_compile_definitions(${TARGET_NAME} PRIVATE "OSMac_")
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

#==============================================================================
# Install
#==============================================================================
install(
    TARGETS ${TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya OPTIONAL)
endif()


#install(
#    DIRECTORY scripts
#    DESTINATION .
#)

#if (INSTALL_MAYA_MOD)
#    install(CODE
#    "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}.mod\"
#\"+ ${TARGET_NAME} any ${CMAKE_INSTALL_PREFIX}
#MAYA_SCRIPT_PATH +:= scripts
#PYTHONPATH +:= scripts
#\")")
#endif ()