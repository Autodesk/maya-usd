# Copyright 2018 Animal Logic
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

list(APPEND DEPENDANT_LIBRARIES AL_USDMaya usdMaya)

set(DIRECTORY_PATH AL/usdmaya/fileio/translators/pxrUsd)

# Setup the plugInfo.json
set(LIBRARY_LOCATION "../../plugin")
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
  @ONLY
)

add_library(${PXR_TRANSLATORS_PACKAGE}
  SHARED
  ProxyShapeTranslator.cpp
  plugin.cpp
)

maya_set_plugin_properties(${PXR_TRANSLATORS_PACKAGE})

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")

    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if (IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    mayaUsd_install_rpath(rpath ${PXR_TRANSLATORS_PACKAGE})
endif()

target_include_directories(
  ${PXR_TRANSLATORS_PACKAGE}
  PUBLIC
  ${PROJECT_SOURCE_DIR}/lib/AL_USDMaya
  ${USD_MAYA_INCLUDE_DIR}
)

target_link_libraries(${PXR_TRANSLATORS_PACKAGE} PRIVATE ${DEPENDANT_LIBRARIES})

install(TARGETS ${PXR_TRANSLATORS_PACKAGE}
 LIBRARY
 DESTINATION ${AL_INSTALL_PREFIX}/plugin
 RUNTIME
 DESTINATION ${AL_INSTALL_PREFIX}/plugin
)
if(IS_WINDOWS)
 install(FILES $<TARGET_PDB_FILE:${PXR_TRANSLATORS_PACKAGE}> DESTINATION ${AL_INSTALL_PREFIX}/plugin OPTIONAL)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
 DESTINATION ${AL_INSTALL_PREFIX}/plugin/${PXR_TRANSLATORS_PACKAGE}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
 "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/plugin/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin/plugInfo.json
 DESTINATION ${AL_INSTALL_PREFIX}/plugin
)

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()