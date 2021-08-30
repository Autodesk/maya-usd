# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        baseExportCommand.cpp
        baseImportCommand.cpp
        baseListShadingModesCommand.cpp
        baseListIOContextsCommand.cpp
        editTargetCommand.cpp
        layerEditorCommand.cpp
        layerEditorWindowCommand.cpp
)

set(HEADERS
        abstractLayerEditorWindow.h
        baseExportCommand.h
        baseImportCommand.h
        baseListShadingModesCommand.h
        baseListIOContextsCommand.h
        editTargetCommand.h
        layerEditorCommand.h
        layerEditorWindowCommand.h
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
