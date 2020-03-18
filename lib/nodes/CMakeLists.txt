# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        hdImagingShape.cpp
        pointBasedDeformerNode.cpp
        proxyShapeBase.cpp
        proxyShapePlugin.cpp
        stageData.cpp
        stageNode.cpp
        usdPrimProvider.cpp
)

set(headers
    hdImagingShape.h
    pointBasedDeformerNode.h
    proxyShapeBase.h
    proxyShapePlugin.h
    stageData.h
    stageNode.h
    usdPrimProvider.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR nodes)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/nodes/
)