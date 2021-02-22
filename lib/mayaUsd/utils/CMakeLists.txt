# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        blockSceneModificationContext.cpp
        colorSpace.cpp
        converter.cpp
        diagnosticDelegate.cpp
        query.cpp
        plugRegistryHelper.cpp
        stageCache.cpp
        undoHelperCommand
        util.cpp
        utilFileSystem.cpp
        utilSerialization.cpp
)

set(HEADERS
    blockSceneModificationContext.h
    colorSpace.h
    converter.h
    diagnosticDelegate.h
    query.h
    plugRegistryHelper.h
    stageCache.h
    undoHelperCommand.h
    util.h
    utilFileSystem.h
    utilSerialization.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR utils)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/utils/
)
