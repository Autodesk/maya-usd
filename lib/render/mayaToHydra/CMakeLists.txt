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

if(CMAKE_WANT_UFE_BUILD)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "${TARGET_NAME}")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "../")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if (IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../USD/lib64")
    endif()
endif()

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (IS_LINUX)
    # gcc will NOT set this automatically
    target_compile_definitions(${TARGET_NAME} PRIVATE "LINUX")
elseif (IS_WINDOWS)
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(${TARGET_NAME} PRIVATE "_WIN32")
endif()
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
