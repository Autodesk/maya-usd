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
        PreferencesApplicationHost.cpp
        AssetResolverDialogCmd.cpp
        AssetResolverProjectChangeTracker.cpp
        AssetResolverUtils.cpp
        PreferencesOptions.cpp
        PreferencesManagement.cpp
        USDAssetResolverDialog.cpp
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        BOOST_NS=PXR_NS::pxr_boost
)

if (ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY)
    target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ADSK_USD_ASSET_RESOLVER_CONTEXTDATA_HAS_PATHARRAY=1
    )
endif()

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(HEADERS
    PreferencesApplicationHost.h
    AssetResolverDialogCmd.h
    AssetResolverProjectChangeTracker.h
    AssetResolverUtils.h
    PreferencesOptions.h
    PreferencesManagement.h
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
