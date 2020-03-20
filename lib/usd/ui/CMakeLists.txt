#
# Copyright 2020 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
project(mayaUsdUI)

set(MAYAUSDUI_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(${PROJECT_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        cmds/USDImportDialogCmd.cpp
        factories/TreeModelFactory.cpp
        images/ui.qrc
        views/ItemDelegate.cpp
        views/TreeItem.cpp
        views/TreeModel.cpp
        views/USDImportDialog.cpp
        views/USDImportDialog.ui
        views/USDQtUtil.cpp
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(mayaUsdUI_headers
    api.h
    cmds/USDImportDialogCmd.h
    factories/TreeModelFactory.h
    views/IMayaMQtUtil.h
    views/IUSDImportView.h
    views/ItemDelegate.h
    views/TreeItem.h
    views/TreeModel.h
    views/USDImportDialog.h
    views/USDQtUtil.h
)

mayaUsd_promoteHeaderList(SUBDIR "ui" HEADERS ${mayaUsdUI_headers})

# -----------------------------------------------------------------------------
# compile definitions
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        MAYAUSD_UI_EXPORT
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${PXR_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        usd
        mayaUsd
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath ${MAYAUSDUI_INSTALL_PATH})
    mayaUsd_install_rpath(rpath ${PROJECT_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(
    TARGETS ${PROJECT_NAME}
    LIBRARY
        DESTINATION ${MAYAUSDUI_INSTALL_PATH}
    ARCHIVE
        DESTINATION ${MAYAUSDUI_INSTALL_PATH}
    RUNTIME
        DESTINATION ${MAYAUSDUI_INSTALL_PATH}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> 
        DESTINATION ${MAYAUSDUI_INSTALL_PATH} OPTIONAL
    )
endif()

install(FILES ${mayaUsdUI_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/ui
)

# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------
if (BUILD_TESTS)
    # A simple executable used to test the import dialog.
    add_subdirectory(demo)
endif()
