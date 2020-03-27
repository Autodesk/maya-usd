# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        shadingModePxrRis.cpp
        shadingModeDisplayColor.cpp
        shadingModeExporter.cpp
        shadingModeExporterContext.cpp
        shadingModeImporter.cpp
        shadingModeRegistry.cpp
        shadingModeUseRegistry.cpp
)

set(headers
    shadingModeExporter.h
    shadingModeExporterContext.h
    shadingModeImporter.h
    shadingModePxrRis_rfm_map.h
    shadingModeRegistry.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS 
        ${headers} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/shading
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/shading
)
