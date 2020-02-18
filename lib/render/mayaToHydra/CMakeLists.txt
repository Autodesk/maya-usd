set(TARGET_NAME mtoh)

set(SRC
    defaultLightDelegate.cpp
    plugin.cpp
    pluginDebugCodes.cpp
    renderGlobals.cpp
    renderOverride.cpp
    shadingModeExporter.cpp
    tokens.cpp
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
    $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
)

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "lib/maya")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if (IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "../")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE 
    $<$<BOOL:${UFE_FOUND}>:WANT_UFE_BUILD>
    # M3dView needs this (technically, MNativeWindowHdl.h)
    # private atm because M3dView is only used in .cpp right now
    # gcc will NOT set this automatically
    $<$<BOOL:${IS_LINUX}>:LINUX>
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    $<$<BOOL:${IS_WINDOWS}>:_WIN32>
)

MAYA_SET_PLUGIN_PROPERTIES(${TARGET_NAME})

#==============================================================================
# Install
#==============================================================================
install(
    TARGETS ${TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya OPTIONAL)
endif()
