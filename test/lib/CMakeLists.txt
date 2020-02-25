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

set(TARGET_NAME MAYAUSD_TEST)

# Unit test scripts.
set(test_script_files
    testMayaUsdPythonImport.py
)

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyFiles(${TARGET_NAME}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${test_script_files})

set(path
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${MAYA_LOCATION}/bin"
    "$ENV{PATH}"
)
separate_argument_list(path)

set(pythonPath
    "${CMAKE_INSTALL_PREFIX}/lib/python"
    "$ENV{PYTHONPATH}"
)
separate_argument_list(pythonPath)

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    add_test(
        NAME ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys; \
                                          from unittest import main; \
                                          import maya.standalone; \
                                          maya.standalone.initialize(name='python'); \
                                          import ${target}; \
                                          testProg = main(module=${target}, exit=False); \
                                          maya.standalone.uninitialize(); \
                                          sys.exit(not testProg.result.wasSuccessful());
                                          "
    )
    set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT
        "PATH=${path}"
        "PYTHONPATH=${pythonPath}"
        "MAYA_NO_STANDALONE_ATEXIT=1"
    )
endforeach()

add_subdirectory(usd)
