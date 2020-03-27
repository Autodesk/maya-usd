# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        batchRenderer.cpp
        debugCodes.cpp
        hdImagingShapeDrawOverride.cpp
        hdImagingShapeUI.cpp
        hdRenderer.cpp
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

set(headers
    batchRenderer.h
    debugCodes.h
    hdImagingShapeDrawOverride.h
    hdImagingShapeUI.h
    hdRenderer.h
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
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR render/pxrUsdMayaGL)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/pxrUsdMayaGL
)
