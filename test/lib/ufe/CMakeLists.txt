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

if(CMAKE_UFE_GROUP_INTERFACE_AVAILABLE)
    list(APPEND test_script_files testGroupCmd.py)
endif()

if(IS_WINDOWS)
    list(APPEND test_script_files testTransform3dTranslate.py)
endif()

# copy ufe tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
file(COPY test-samples DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ufeScripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ufeTestUtils DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
foreach(test_file ${test_script_files})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${test_file} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

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
    get_unittest_target(target ${script})
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
    if(IS_LINUX)
        set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT 
            "LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/lib:${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
        )
    endif()
endforeach()
