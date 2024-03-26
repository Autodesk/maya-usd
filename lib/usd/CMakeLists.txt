#
# Copyright 2023 Autodesk
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
if(BUILD_HDMAYA)
    add_subdirectory(hdMaya)
endif()

add_subdirectory(pxrUsdPreviewSurface)
add_subdirectory(translators)
add_subdirectory(schemas)

if(Qt_FOUND)
    add_subdirectory(ui)
endif()

#install top level plugInfo.json that includes the configured plugInfo.json
set(INSTALL_DESTINATION 
    ${CMAKE_INSTALL_PREFIX}/lib/usd
)
install(CODE
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
    DESTINATION ${INSTALL_DESTINATION}
)
