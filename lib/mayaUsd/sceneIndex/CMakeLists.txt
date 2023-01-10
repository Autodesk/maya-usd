# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        proxyShapeSceneIndexPlugin.cpp
)

set(HEADERS
    proxyShapeSceneIndexPlugin.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR sceneIndex)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/sceneIndex
)

# -----------------------------------------------------------------------------
# plugInfo
# -----------------------------------------------------------------------------

if(IS_WINDOWS)
    set(PLUG_INFO_LIBRARY_PATH "mayaUsd.dll")
elseif(IS_MACOSX)
    set(PLUG_INFO_LIBRARY_PATH "mayaUsd.dylib")
elseif(IS_LINUX)
    set(PLUG_INFO_LIBRARY_PATH "mayaUsd.so")
endif()

set(PLUG_INFO_DIR ${CMAKE_INSTALL_PREFIX}/lib/usd/sceneIndex/resources)
set(PLUG_INFO_ROOT ${CMAKE_INSTALL_PREFIX}/lib)

configure_file(
    plugInfo.json
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${PLUG_INFO_DIR}
)

