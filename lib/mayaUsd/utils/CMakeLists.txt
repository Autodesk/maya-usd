# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        blockSceneModificationContext.cpp
        colorSpace.cpp
        diagnosticDelegate.cpp
        query.cpp
        stageCache.cpp
        undoHelperCommand
        util.cpp
        utilFileSystem.cpp
)

set(headers
    blockSceneModificationContext.h
    colorSpace.h
    diagnosticDelegate.h
    query.h
    stageCache.h
    undoHelperCommand.h
    util.h
    utilFileSystem.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR utils)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/utils/
)