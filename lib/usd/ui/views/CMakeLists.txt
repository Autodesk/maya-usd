# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        TreeItem.cpp
        ItemDelegate.cpp
        TreeModel.cpp
        USDImportDialog.cpp
        USDQtUtil.cpp
        USDImportDialog.ui
)

set(headers
    IMayaMQtUtil.h
    ItemDelegate.h
    IUSDImportView.h
    TreeItem.h
    TreeModel.h
    USDImportDialog.h
    USDQtUtil.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList( 
    HEADERS
        ${headers}
    BASEDIR
        ${PROJECT_NAME}/ui/views
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ui/views
)