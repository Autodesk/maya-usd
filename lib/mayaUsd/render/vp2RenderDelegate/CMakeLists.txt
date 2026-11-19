# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        basisCurves.cpp
        bboxGeom.cpp
        debugCodes.cpp
        drawItem.cpp
        extComputation.cpp
        instancer.cpp
        material.cpp
        mayaPrimCommon.cpp
        mesh.cpp
        meshViewportCompute.cpp
        points.cpp
        proxyRenderDelegate.cpp
        colorManagementPreferences.cpp
        renderDelegate.cpp
        renderParam.cpp
        sampler.cpp
        shader.cpp
        tokens.cpp
        usdImagingDelegateProducer.cpp
        usdProducer.cpp
)

if(CMAKE_WANT_MAYAUSD_VP2_USE_SCENE_INDEX)
    target_sources(${PROJECT_NAME}
        PRIVATE
            sceneIndexProducer.cpp
            sceneIndex/subtreeScopingSceneIndex.cpp
    )
endif()

# Gaussian splat support needs USD 26.05 or later (HdPrimTypeTokens->particleField).
# This is the only place that checks the USD version for splats. Splat sources are added
# here, and renderDelegate.cpp tests the compile definition.
if(PXR_VERSION GREATER_EQUAL 2605)
    target_sources(${PROJECT_NAME}
        PRIVATE
            gaussianSplat.cpp
            gaussianSplatCallback.cpp
            gaussianSplatViewCache.cpp
            gaussianSplatViewCore.cpp
    )
    # The splat sort uses WorkParallelForN (pxr/base/work).
    target_link_libraries(${PROJECT_NAME} PRIVATE work)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            MAYAUSD_ENABLE_GAUSSIAN_SPLAT
    )
endif()

set(HEADERS
    proxyRenderDelegate.h
    colorManagementPreferences.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/vp2RenderDelegate)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/vp2RenderDelegate
)

set(PYTHON_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME})
install(FILES analyticMayaUsdPerformance.py DESTINATION ${PYTHON_INSTALL_PREFIX})
