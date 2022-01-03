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
