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
    hdMaya 
    usdShade
    ${MAYA_LIBRARIES}
)

target_include_directories(${TARGET_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${MAYA_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/usd
)

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if (IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        add_rpath(rpath "../../../USD/lib64")
    endif()
endif()

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (IS_LINUX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
    # gcc will NOT set this automatically
    target_compile_definitions(${TARGET_NAME} PRIVATE "LINUX")
elseif (IS_WINDOWS)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".mll")
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(${TARGET_NAME} PRIVATE "_WIN32")
elseif (IS_MACOSX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".bundle")
    # Not sure if osx sets this automatically, but won't hurt to redefine
    target_compile_definitions(${TARGET_NAME} PRIVATE "OSMac_")
endif()

#==============================================================================
# Install
#==============================================================================
install(
    TARGETS ${TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya)

if(IS_WINDOWS)
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
