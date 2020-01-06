# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        proxyShapeBase.cpp
        proxyShapePlugin.cpp
        stageData.cpp
        usdPrimProvider.cpp
)

set(headers
    proxyShapeBase.h
    proxyShapePlugin.h
    stageData.h
    usdPrimProvider.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR nodes)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/nodes/
)