# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        editability.cpp
        editRouter.cpp
        editRouterContext.cpp
        layers.cpp
        loadRules.cpp
        loadRulesText.cpp
		uiCallback.cpp
        usdUtils.cpp
        Utils.cpp
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(HEADERS
    editability.h
    editRouter.h
    editRouterContext.h
    layers.h
    loadRules.h
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
