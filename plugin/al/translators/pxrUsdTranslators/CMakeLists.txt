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

MAYA_SET_PLUGIN_PROPERTIES(${PXR_TRANSLATORS_PACKAGE})

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")

    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()

    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    mayaUsd_install_rpath(rpath ${PXR_TRANSLATORS_PACKAGE})
endif()

if (IS_LINUX)
  if(WANT_USD_RELATIVE_PATH)
      mayaUsd_add_rpath(rpath "../../../../USD/lib64")
  endif()
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
 DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
 RUNTIME
 DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)
if(IS_WINDOWS)
 install(FILES $<TARGET_PDB_FILE:${PXR_TRANSLATORS_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
 DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/${PXR_TRANSLATORS_PACKAGE}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
 "file(WRITE \"${CMAKE_INSTALL_PREFIX}/plugin/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()