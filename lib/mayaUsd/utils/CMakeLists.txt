# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        autoUndoCommands.cpp
        blockSceneModificationContext.cpp
        colorSpace.cpp
        converter.cpp
        copyLayerPrims.cpp
        customLayerData.cpp
        diagnosticDelegate.cpp
        dynamicAttribute.cpp
        json.cpp
        layerLocking.cpp
        layerMuting.cpp
        layers.cpp
        loadRulesAttribute.cpp
        mayaEditRouter.cpp
        mayaNodeObserver.cpp
        mayaNodeTypeObserver.cpp
        query.cpp
        plugRegistryHelper.cpp
        primActivation.cpp
        progressBarScope.cpp
        selectability.cpp
        stageCache.cpp
        targetLayer.cpp
        traverseLayer.cpp
        undoHelperCommand.cpp
        utilComponentCreator.cpp
        util.cpp
        utilDictionary.cpp
        utilFileSystem.cpp
        utilSerialization.cpp
        variants.cpp
        variantFallbacks.cpp
)

set(HEADERS
    autoUndoCommands.h
    blockSceneModificationContext.h
    colorSpace.h
    customLayerData.h
    converter.h
    copyLayerPrims.h
    diagnosticDelegate.h
    dynamicAttribute.h
    hash.h
    json.h
    jsonConverter.h
    layerLocking.h
    layerMuting.h
    layers.h
    loadRules.h
    mayaEditRouter.h
    mayaNodeObserver.h
    mayaNodeTypeObserver.h
    query.h
    plugRegistryHelper.h
    primActivation.h
    progressBarScope.h
    selectability.h
    stageCache.h
    targetLayer.h
    traverseLayer.h
    trieVisitor.h
    undoHelperCommand.h
    utilComponentCreator.h
    util.h
    utilDictionary.h
    utilFileSystem.h
    utilSerialization.h
    variants.h
    variantFallbacks.h
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

