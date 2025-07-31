#
# Copyright 2024 Autodesk
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

set(TARGET_NAME exampleImportExportPlugin)
add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        helpers.cpp
        helpersJSON.cpp
        helpersUI.cpp
        exampleExportPlugin.cpp
        exampleImportPlugin.cpp
)
mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        _USE_MATH_DEFINES
)

# -----------------------------------------------------------------------------
# Qt support
# -----------------------------------------------------------------------------

# QT_NO_KEYWORDS prevents Qt from defining the foreach, signals, slots and emit macros.
# this avoids overlap between Qt macros, and enforces using Q_ macros.
get_target_property(qt_core_aliased Qt::Core ALIASED_TARGET)
if (qt_core_aliased)
    set_target_properties(${qt_core_aliased} PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
else()
    set_target_properties(Qt::Core PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
endif()

target_link_libraries(${TARGET_NAME}
    PRIVATE
		Qt::Core
		Qt::Gui
		Qt::Widgets
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
)

maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
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
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# Configure the plugin JSON to its final destination
# -----------------------------------------------------------------------------
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json"
    "${CMAKE_CURRENT_BINARY_DIR}/exampleImportExportPlugin/plugInfo.json"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/mayaUsdPlugInfo.json"
    "${CMAKE_CURRENT_BINARY_DIR}/mayaUsdPlugInfo.json"
)
