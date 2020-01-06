# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
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
    proxyRenderDelegate.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR render/vp2RenderDelegate)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/render/vp2RenderDelegate
)
