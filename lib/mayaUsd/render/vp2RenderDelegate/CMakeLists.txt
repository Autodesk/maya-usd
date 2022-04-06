# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        basisCurves.cpp
        bboxGeom.cpp
        debugCodes.cpp
        draw_item.cpp
        extComputation.cpp
        instancer.cpp
        material.cpp
        mayaPrimCommon.cpp
        mesh.cpp
        meshViewportCompute.cpp
        points.cpp
        proxyRenderDelegate.cpp
        render_delegate.cpp
        render_param.cpp
        sampler.cpp
        shader.cpp
        tokens.cpp
)

set(HEADERS
    proxyRenderDelegate.h
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
