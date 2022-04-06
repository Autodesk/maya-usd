# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        blockSceneModificationContext.cpp
        colorSpace.cpp
        converter.cpp
        customLayerData.cpp
        diagnosticDelegate.cpp
        editability.cpp
        editRouter.cpp
        loadRules.cpp
        loadRulesText.cpp
        loadRulesAttribute.cpp
        query.cpp
        plugRegistryHelper.cpp
        selectability.cpp
        stageCache.cpp
        traverseLayer.cpp
        undoHelperCommand.cpp
        util.cpp
        utilFileSystem.cpp
        utilSerialization.cpp
)

set(HEADERS
    blockSceneModificationContext.h
    colorSpace.h
    customLayerData.h
    converter.h
    diagnosticDelegate.h
    editability.h
    editRouter.h
    hash.h
    loadRules.h
    query.h
    plugRegistryHelper.h
    selectability.h
    stageCache.h
    traverseLayer.h
    undoHelperCommand.h
    util.h
    utilFileSystem.h
    utilSerialization.h
)

set(PLUGINFO
    plugInfo.json)

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

install(FILES ${PLUGINFO}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_Utils/resources
)

