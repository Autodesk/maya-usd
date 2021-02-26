#
# Copyright 2021 Autodesk
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

target_sources(${PROJECT_NAME}
    PRIVATE
        ItemDelegate.cpp
        TreeItem.cpp
        TreeModel.cpp
        TreeModelFactory.cpp
        USDImportDialog.cpp
        USDImportDialogCmd.cpp
        USDQtUtil.cpp

        USDImportDialog.ui
        images/ui.qrc
)


# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(HEADERS
    IMayaMQtUtil.h
    ItemDelegate.h
    IUSDImportView.h
    TreeItem.h
    TreeModel.h
    TreeModelFactory.h
    USDImportDialog.h
    USDImportDialogCmd.h
    USDQtUtil.h
)

mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}/ui
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/ui
)
