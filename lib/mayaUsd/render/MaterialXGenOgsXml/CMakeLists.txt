# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        GlslFragmentGenerator.cpp
        OgsFragment.cpp
        OgsXmlGenerator.cpp
        Nodes/SurfaceNodeMaya.cpp
        PugiXML/pugixml.cpp
)

set(HEADERS
    GlslFragmentGenerator.h
    OgsFragment.h
    OgsXmlGenerator.h
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
list(APPEND LIGHT_IMPLEMENTATIONS
    libraries/mx_lighting_maya_v1.glsl
    libraries/mx_lighting_maya_v2.glsl
)

list(APPEND NODE_DECLARATIONS
    libraries/maya_surfaces.mtlx
    libraries/maya_utilities.mtlx
    libraries/usd_utilities.mtlx
    libraries/lookdevKit.mtlx
    libraries/lookdevKit_ng.mtlx
)

list(APPEND NODE_GLSL_IMPLEMENTATIONS
    libraries/usd_utilities_genglsl_impl.mtlx
    libraries/mx_texcoordtangents_vector3.glsl
    libraries/mx_arbitrarytangents_vector3.glsl
    libraries/mx_srgb_texture_to_aces_2065_1_color3.glsl
    libraries/mx_srgb_texture_to_aces_2065_1_color4.glsl
    libraries/mx_srgb_texture_to_acescg_color3.glsl
    libraries/mx_srgb_texture_to_acescg_color4.glsl
    libraries/mx_srgb_texture_to_lin_dci_p3_d65_color3.glsl
    libraries/mx_srgb_texture_to_lin_dci_p3_d65_color4.glsl
    libraries/mx_srgb_texture_to_linrec2020_color3.glsl
    libraries/mx_srgb_texture_to_linrec2020_color4.glsl
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/MaterialXGenOgsXml)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${LIGHT_IMPLEMENTATIONS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/pbrlib/genglsl/ogsxml
)

install(FILES ${NODE_DECLARATIONS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya
)

install(FILES ${NODE_GLSL_IMPLEMENTATIONS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
)
