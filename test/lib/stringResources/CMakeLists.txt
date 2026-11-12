#
# Copyright 2026 Autodesk
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

# -----------------------------------------------------------------------------
# C++ unit test over the string lists of all four libraries (UsdUfe, UsdLayerEditor, MayaUsd and
# MayaUsdUI). It only reads the headers, but needs all of their include paths.
# -----------------------------------------------------------------------------
set(TARGET_NAME testStringResourceKeys)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        testStringResourceKeys.cpp
)

mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        GTest::GTest
        Qt::Core
        ${MAYA_LIBRARIES}
        mayaUsd
        mayaUsdUI
        usdLayerEditorUi
        usdUfe
)

mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
)

# Add a ctest label to these tests for easy filtering.
set_property(TEST ${TARGET_NAME} APPEND PROPERTY LABELS StringResource)
