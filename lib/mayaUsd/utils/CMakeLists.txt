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
        dynamicAttribute.cpp
        editability.cpp
        editRouter.cpp
        layerMuting.cpp
        loadRules.cpp
        loadRulesText.cpp
        loadRulesAttribute.cpp
        query.cpp
        plugRegistryHelper.cpp
        progressBarScope.cpp
        selectability.cpp
        stageCache.cpp
        traverseLayer.cpp
        undoHelperCommand.cpp
        util.cpp
        utilDictionary.cpp
        utilFileSystem.cpp
        utilSerialization.cpp
        variants.cpp
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            json.cpp
            primActivation.cpp
    )
endif()

set(HEADERS
    blockSceneModificationContext.h
    colorSpace.h
    customLayerData.h
    converter.h
    diagnosticDelegate.h
    dynamicAttribute.h
    editability.h
    editRouter.h
    hash.h
    layerMuting.h
    loadRules.h
    query.h
    plugRegistryHelper.h
    progressBarScope.h
    selectability.h
    stageCache.h
    traverseLayer.h
    undoHelperCommand.h
    util.h
    utilDictionary.h
    utilFileSystem.h
    utilSerialization.h
    variants.h
)
if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        json.h
        primActivation.h
    )
endif()

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

