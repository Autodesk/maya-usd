# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        shadingModeDisplayColor.cpp
        shadingModeExporter.cpp
        shadingModeExporterContext.cpp
        shadingModeImporter.cpp
        shadingModePxrRis.cpp
        shadingModeRegistry.cpp
        shadingModeUseRegistry.cpp
        symmetricShaderReader.cpp
        symmetricShaderWriter.cpp
)

set(HEADERS
    rfmShaderMap.h
    shadingModeExporter.h
    shadingModeExporterContext.h
    shadingModeImporter.h
    shadingModeRegistry.h
    symmetricShaderReader.h
    symmetricShaderWriter.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS 
        ${HEADERS} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/shading
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/shading
)
