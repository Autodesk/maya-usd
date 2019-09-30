#
# Copyright 2019 Autodesk
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

set(TARGET_NAME UFE_TEST)

# unit test scripts
set(test_script_files
    testDeleteCmd.py
    testDuplicateCmd.py
    testMatrices.py
    testMayaPickwalk.py
    testMoveCmd.py
    testRotateCmd.py
    testRotatePivot.py
    testScaleCmd.py
    testTRSBase.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND test_script_files testGroupCmd.py)
endif()

if(IS_WINDOWS)
    list(APPEND test_script_files testTransform3dTranslate.py)
endif()

# copy ufe tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there

add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY test-samples)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeScripts)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeTestUtils)
mayaUsd_copyFiles(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                         FILES ${test_script_files})

# unit tests
set(pythonPath
    "${CMAKE_INSTALL_PREFIX}/lib/python"
    "$ENV{PYTHONPATH}"
)

set(path 
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${MAYA_LOCATION}/bin"
    "$ENV{PATH}"
)

if(IS_WINDOWS)
    string(REPLACE ";" "\;" pythonPath "${pythonPath}")
    string(REPLACE ";" "\;" path "${path}")
else()
    separate_arguments(pythonPath NATIVE_COMMAND "${pythonPath}")
    separate_arguments(path NATIVE_COMMAND "${path}")

    string(REPLACE "\;" ":" pythonPath "${pythonPath}")
    string(REPLACE "\;" ":" path "${path}")
endif()

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    add_test(
        NAME ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${MAYA_PY_EXECUTABLE} -c "from unittest import main;import maya.standalone;maya.standalone.initialize(name='python');import ${target};main(module=${target}, exit=False);maya.standalone.uninitialize()"
    )
    set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT 
        "PYTHONPATH=${pythonPath}"
        "PATH=${path}"
        "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/plugin/adsk/plugin"
        "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
    )
endforeach()
