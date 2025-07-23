# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        GlslFragmentGenerator.cpp
        GlslOcioNodeImpl.cpp
        OgsFragment.cpp
        OgsXmlGenerator.cpp
        ShaderGenUtil.cpp
        LobePruner.cpp
        Nodes/SurfaceNodeMaya.cpp
        Nodes/TexcoordNodeMaya.cpp
        Nodes/MayaDarkClosureNode.cpp
        PugiXML/pugixml.cpp
)

if(MaterialX_VERSION VERSION_LESS "1.38.9")
target_sources(${PROJECT_NAME} 
    PRIVATE
        Nodes/MayaTransformNormalNodeGlsl.cpp
        Nodes/MayaTransformPointNodeGlsl.cpp
        Nodes/MayaTransformVectorNodeGlsl.cpp
)
endif()

if(MaterialX_VERSION VERSION_GREATER_EQUAL "1.38.8")
# Locally fixing the UV scale and offset for UDIMs. We will submit to 1.39 later.
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    FIX_NODEGRAPH_UDIM_SCALE_OFFSET
    )
target_sources(${PROJECT_NAME} 
    PRIVATE
        Nodes/MayaHwImageNode.cpp
        Nodes/MayaCompoundNode.cpp
        Nodes/MayaShaderGraph.cpp
    )
endif()

if(MaterialX_VERSION VERSION_GREATER_EQUAL "1.38.8" AND MaterialX_VERSION VERSION_LESS "1.39.0")
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    FIX_DUPLICATE_INCLUDED_SHADER_CODE
    )
target_sources(${PROJECT_NAME} 
    PRIVATE
        Nodes/MayaSourceCodeNode.cpp
    )
endif()

if(USD_HAS_BACKPORTED_MX39_OPENPBR)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    USD_HAS_BACKPORTED_MX39_OPENPBR
    )
target_sources(${PROJECT_NAME} 
    PRIVATE
        Nodes/MayaClosureSourceCodeNode.cpp
    )
endif()

set(HEADERS
    CombinedMaterialXVersion.h
    GlslFragmentGenerator.h
    GlslOcioNodeImpl.h
    OgsFragment.h
    OgsXmlGenerator.h
    ShaderGenUtil.h
    LobePruner.h
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
list(APPEND LIGHT_IMPLEMENTATIONS
    libraries/mx_lighting_maya_none.glsl
    libraries/mx_lighting_maya_v1.glsl
    libraries/mx_lighting_maya_v2.glsl
    libraries/mx_lighting_maya_v3.glsl
)

if(USD_HAS_BACKPORTED_MX39_OPENPBR)
    list(APPEND LIGHT_IMPLEMENTATIONS
        libraries/mx39_lighting_maya_all.glsl
    )
endif()

list(APPEND NODE_DECLARATIONS
    libraries/maya_surfaces.mtlx
    libraries/maya_utilities.mtlx
    libraries/maya_utilities_ng.mtlx
    libraries/usd_utilities.mtlx
    libraries/lookdevKit.mtlx
    libraries/lookdevKit_ng.mtlx
)

list(APPEND NODE_GLSL_IMPLEMENTATIONS
    libraries/mx_texcoordtangents_vector3.glsl
    libraries/mx_arbitrarytangents_vector3.glsl
)

list(APPEND NODE_GLSL_IMPLEMENTATIONS_VERSIONED
    libraries/usd_utilities_genglsl_impl.mtlx
    libraries/mx_srgb_texture_to_aces_2065_1_color3.glsl
    libraries/mx_srgb_texture_to_aces_2065_1_color4.glsl
    libraries/mx_srgb_texture_to_acescg_color3.glsl
    libraries/mx_srgb_texture_to_acescg_color4.glsl
    libraries/mx_srgb_texture_to_lin_dci_p3_d65_color3.glsl
    libraries/mx_srgb_texture_to_lin_dci_p3_d65_color4.glsl
    libraries/mx_srgb_texture_to_linrec2020_color3.glsl
    libraries/mx_srgb_texture_to_linrec2020_color4.glsl
)

list(APPEND NODE_GLSL_LIB_VERSIONED
    libraries/usd_transform_color.glsl
)

list(APPEND NODE_GLSL_EXTRA_IMPLEMENTATIONS_VERSIONED
    libraries/mx_srgb_texture_to_lin_rec709_color3.glsl
    libraries/mx_srgb_texture_to_lin_rec709_color4.glsl
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/MaterialXGenOgsXml)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/MaterialXGenOgsXml
)

install(FILES ${NODE_DECLARATIONS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya
)

install(FILES ${NODE_GLSL_IMPLEMENTATIONS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
)

if(MaterialX_VERSION VERSION_LESS "1.38.4")
    MESSAGE(STATUS "Using 1.38.3 libraries with MaterialX ${MaterialX_VERSION}")
    foreach(MTLX_FILE ${NODE_GLSL_IMPLEMENTATIONS_VERSIONED})
        string(REPLACE "libraries/" "libraries/1.38.3/" MTLX_FILE ${MTLX_FILE})
        install(FILES "${MTLX_FILE}"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
        )
    endforeach()
    foreach(MTLX_FILE ${LIGHT_IMPLEMENTATIONS})
        string(REPLACE "libraries/" "libraries/1.38.3/" MTLX_FILE ${MTLX_FILE})
        install(FILES "${MTLX_FILE}"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/pbrlib/genglsl/ogsxml
        )
    endforeach()
elseif(MaterialX_VERSION VERSION_LESS "1.38.8")
    MESSAGE(STATUS "Using 1.38.[4-7] libraries with MaterialX ${MaterialX_VERSION}")
    foreach(MTLX_FILE ${NODE_GLSL_IMPLEMENTATIONS_VERSIONED})
        string(REPLACE "libraries/" "libraries/1.38.7/" MTLX_FILE ${MTLX_FILE})
        install(FILES "${MTLX_FILE}"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
        )
    endforeach()
    foreach(MTLX_FILE ${LIGHT_IMPLEMENTATIONS})
        string(REPLACE "libraries/" "libraries/1.38.7/" MTLX_FILE ${MTLX_FILE})
        install(FILES "${MTLX_FILE}"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/pbrlib/genglsl/ogsxml
        )
    endforeach()
else()
    MESSAGE(STATUS "Using latest libraries with MaterialX ${MaterialX_VERSION}")
    install(FILES ${NODE_GLSL_IMPLEMENTATIONS_VERSIONED}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
    )
    install(FILES ${NODE_GLSL_EXTRA_IMPLEMENTATIONS_VERSIONED}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl
    )
    install(FILES ${NODE_GLSL_LIB_VERSIONED}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/adsk/maya/genglsl/lib
    )
    install(FILES ${LIGHT_IMPLEMENTATIONS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libraries/pbrlib/genglsl/ogsxml
    )
endif()
