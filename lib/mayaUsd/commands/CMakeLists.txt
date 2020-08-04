# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        baseExportCommand.cpp
        baseImportCommand.cpp
        baseListShadingModesCommand.cpp
        editTargetCommand.cpp
        layerEditorCommand.cpp
)

set(HEADERS
        baseExportCommand.h
        baseImportCommand.h
        baseListShadingModesCommand.h
        editTargetCommand.h
        layerEditorCommand.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR commands)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/commands/
)
