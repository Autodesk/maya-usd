#
# Copyright 2025 Autodesk
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
        ApplicationHost.cpp
        HeaderWidget.cpp
        USDAssetResolverSettingsWidget.cpp
        AssetResolverDialogCmd.cpp
        USDAssetResolverDialog.cpp
        USDAssetResolverSettingsWidget.ui
        images/ui.qrc
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        BOOST_NS=PXR_INTERNAL_NS::pxr_boost
)

if (AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY=1
    )
endif()

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(HEADERS
    ApplicationHost.h
    HeaderWidget.h
    USDAssetResolverSettingsWidget.h
    AssetResolverDialogCmd.h
    USDAssetResolverDialog.h
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
