# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        hdImagingShape.cpp
        layerManager.cpp
        pointBasedDeformerNode.cpp
        proxyAccessor.cpp
        proxyShapeBase.cpp
        proxyShapePlugin.cpp
        proxyShapeStageExtraData.cpp
        proxyShapeListenerBase.cpp
        stageData.cpp
        stageNode.cpp
        usdPrimProvider.cpp
)

if(AdskUsdEditForward_FOUND)
    target_sources(${PROJECT_NAME}
        PRIVATE
            MayaUsdEditForwardCommand.cpp
            MayaUsdEditForwardHost.cpp
    )
endif()

set(HEADERS
    hdImagingShape.h
    layerManager.h
    pointBasedDeformerNode.h
    proxyAccessor.h
    proxyShapeBase.h
    proxyShapePlugin.h
    proxyStageProvider.h
    proxyShapeStageExtraData.h
    proxyShapeListenerBase.h
    stageData.h
    stageNode.h
    usdPrimProvider.h
)

if(AdskUsdEditForward_FOUND)
    list(APPEND HEADERS
        MayaUsdEditForwardCommand.h
        MayaUsdEditForwardHost.h
    )
endif()

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR nodes)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/nodes/
)

set(PYTHON_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/lib)
install(FILES proxyAccessor.py DESTINATION ${PYTHON_INSTALL_PREFIX})
