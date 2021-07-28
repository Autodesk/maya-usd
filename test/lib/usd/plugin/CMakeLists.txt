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
set(TARGET_NAME usdTestMayaPlugin)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        mayaShaderTranslation.cpp
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
        usdShade
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# Plug-in files
# -----------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plugInfoMaya.json"
               "${CMAKE_CURRENT_BINARY_DIR}/Maya/plugInfo.json"
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

set(TARGET_NAME usdTestUsdPlugin)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        mayaShaderTranslation.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
        usdShade
)

# -----------------------------------------------------------------------------
# Plug-in files
# -----------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plugInfoUsd.json"
               "${CMAKE_CURRENT_BINARY_DIR}/USD/plugInfo.json"
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# Plug Plug-in files
# -----------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mayaUsdPlugInfo.json"
               "${CMAKE_CURRENT_BINARY_DIR}/TestMayaUsdPlug/mayaUsdPlugInfo.json"
)


set(TARGET_NAME usdTestInfoImportChaser)
add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        infoImportChaser.cpp
)
mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()


# -----------------------------------------------------------------------------
# Plug Plug-ins
# -----------------------------------------------------------------------------
set(PLUGPLUGIN_NAMES
    testPlugModule1
    testPlugModule2
    testPlugModule3
    testPlugModule4
    testPlugModule5
    testPlugModule6
)

foreach(plugName ${PLUGPLUGIN_NAMES})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${plugName}.py"
            RENAME __init__.py
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/TestMayaUsdPlug/${plugName}")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${plugName}_plugInfo.json"
                   "${CMAKE_CURRENT_BINARY_DIR}/TestMayaUsdPlug/${plugName}/plugInfo.json"
)
endforeach()

set(TARGET_NAME usdTestApiWriter)
add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        nullApiExporter.cpp
)
mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/nullApiExporter_plugInfo.json"
                "${CMAKE_CURRENT_BINARY_DIR}/nullApiExporter/plugInfo.json"
)
