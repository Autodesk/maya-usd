# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        editRouter.cpp
        editRouterContext.cpp
        layers.cpp
        loadRules.cpp
        loadRulesText.cpp
        usdUtils.cpp
        Utils.cpp
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    editRouter.h
    editRouterContext.h
    layers.h
    loadRules.h
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
