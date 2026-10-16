
add_subdirectory(rendering)

target_sources(${PROJECT_NAME}
    PRIVATE
        mayaEditCommitter.cpp
        mayaRendererProvider.cpp
        mayaRenderSetupHost.cpp
        renderLayers.cpp
        renderSetupWindowCmd.cpp
)

mayaUsd_promoteHeaderList(
    HEADERS
        mayaEditCommitter.h
        mayaRendererProvider.h
        mayaRenderSetupHost.h
        renderLayers.h
        renderSetupWindowCmd.h
    BASEDIR
        ${PROJECT_NAME}/ui
)
