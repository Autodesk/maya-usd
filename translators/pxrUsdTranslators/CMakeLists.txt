# Copyright 2018 Animal Logic
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.//
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(NOT USD_MAYA_LIBRARY)
    message(FATAL_ERROR
        "Cannot compile ${PXR_TRANSLATORS_PACKAGE} - could not find usdMaya library\n\
Either set USD_MAYA_ROOT so it may be found, or disable building this package by setting BUILD_USDMAYA_PXR_TRANSLATORS to false")
endif()

list(APPEND DEPENDANT_LIBRARIES AL_USDMaya "${USD_MAYA_LIBRARY}")

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

if(MSVC)
    set_target_properties(${PXR_TRANSLATORS_PACKAGE} PROPERTIES SUFFIX ".mll")
else()
    set(rpath "$ORIGIN:$ORIGIN/../lib")
    if (CMAKE_INSTALL_RPATH)
        set(rpath "${CMAKE_INSTALL_RPATH}:${rpath}")
    endif()
    set_target_properties(${PXR_TRANSLATORS_PACKAGE}
            PROPERTIES
            PREFIX ""
            INSTALL_RPATH "${rpath}"
    )
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
 DESTINATION plugin
 RUNTIME
 DESTINATION plugin
)

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