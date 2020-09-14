# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        batchRenderer.cpp
        debugCodes.cpp
        hdImagingShapeDrawOverride.cpp
        hdImagingShapeUI.cpp
        instancerImager.cpp
        instancerShapeAdapter.cpp
        proxyDrawOverride.cpp
        proxyShapeDelegate.cpp
        proxyShapeUI.cpp
        sceneDelegate.cpp
        shapeAdapter.cpp
        softSelectHelper.cpp
        usdProxyShapeAdapter.cpp
        userData.cpp
)

set(HEADERS
    batchRenderer.h
    debugCodes.h
    hdImagingShapeDrawOverride.h
    hdImagingShapeUI.h
    instancerImager.h
    instancerShapeAdapter.h
    proxyDrawOverride.h
    proxyShapeUI.h
    renderParams.h
    sceneDelegate.h
    shapeAdapter.h
    softSelectHelper.h
    usdProxyShapeAdapter.h
    userData.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/pxrUsdMayaGL)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/pxrUsdMayaGL
)
