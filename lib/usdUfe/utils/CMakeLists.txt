# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        diffAttributes.cpp
        diffCore.cpp
        diffDictionaries.cpp
        diffLists.cpp
        diffMetadatas.cpp
        diffPrims.cpp
        diffRelationships.cpp
        diffValues.cpp
        editRouter.cpp
        editRouterContext.cpp
        layers.cpp
        loadRules.cpp
        loadRulesText.cpp
        mergePrims.cpp
        mergePrimsOptions.cpp
        uiCallback.cpp
        usdUtils.cpp
        Utils.cpp
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    ALHalf.h
    diffCore.h
    diffPrims.h
    editRouter.h
    editRouterContext.h
    layers.h
    loadRules.h
    mergePrims.h
    mergePrimsOptions.h
    SIMD.h
    uiCallback.h
    usdUtils.h
    Utils.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR utils)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/utils/
)
