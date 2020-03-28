# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        basisCurves.cpp
        bboxGeom.cpp
        debugCodes.cpp
        draw_item.cpp
        instancer.cpp
        material.cpp
        mesh.cpp
        proxyRenderDelegate.cpp
        render_delegate.cpp
        render_param.cpp
        sampler.cpp
        tokens.cpp
)

set(headers
    basisCurves.h
    bboxGeom.h
    debugCodes.h
    draw_item.h
    instancer.h
    material.h
    mesh.h
    proxyRenderDelegate.h
    render_delegate.h
    render_param.h
    render_pass.h
    resource_registry.h
    sampler.h
    task_commit.h
    tokens.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR render/vp2RenderDelegate)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/vp2RenderDelegate
)
