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
# C++ unit tests
#
# MayaRendererProvider talks to a live Maya session (MEL "renderer" queries,
# defaultRenderGlobals) and to USD's Hydra plugin registry, so this test
# brings up Maya in library ("standalone") mode itself (see main.cpp) rather
# than relying on Python's maya.standalone, which the other C++ tests under
# test/lib/mayaUsd/utils don't need.
# -----------------------------------------------------------------------------
if(IS_WINDOWS AND MAYA_HAS_USD_SETTINGS_NODES)
    # Mirrors the Windows-only gating used for the other C++ tests that link
    # Maya + USD together (see test/lib/mayaUsd/utils/CMakeLists.txt).
    add_executable(testMayaRendererProvider)

    target_sources(testMayaRendererProvider
        PRIVATE
        main.cpp
        testMayaRendererProvider.cpp
    )

    mayaUsd_compile_config(testMayaRendererProvider)

    target_include_directories(testMayaRendererProvider
        PRIVATE
        ${ADSK_USD_RENDER_SETUP_INCLUDE_DIR}
    )

    target_link_libraries(testMayaRendererProvider
        PRIVATE
        GTest::GTest
        ${MAYA_LIBRARIES}
        mayaUsd
        mayaUsdUI
    )

    mayaUsd_add_test(testMayaRendererProvider
        COMMAND $<TARGET_FILE:testMayaRendererProvider>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )

    set_property(TEST testMayaRendererProvider APPEND PROPERTY LABELS SharedComponents)
endif()
