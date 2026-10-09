
add_subdirectory(rendering)

target_sources(${PROJECT_NAME}
    PRIVATE
        mayaEditCommitter.cpp
        mayaRendererProvider.cpp
        mayaRenderSetupHost.cpp
        renderSetupWindowCmd.cpp
)

mayaUsd_promoteHeaderList(
    HEADERS
        mayaEditCommitter.h
        mayaRendererProvider.h
        mayaRenderSetupHost.h
        renderSetupWindowCmd.h
    BASEDIR
        ${PROJECT_NAME}/ui
)
