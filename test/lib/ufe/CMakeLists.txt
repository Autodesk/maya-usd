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

# unit test scripts.  Note that testTRSBase.py is not a test case, but rather
# a module providing a base class for other tests.
set(test_script_files
    testDeleteCmd.py
    testMatrices.py
    testMayaPickwalk.py
    testRotatePivot.py
)

set(test_support_files
    testTRSBase.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND test_script_files
        testGroupCmd.py
        testAttribute.py
        testAttributes.py
		# The following files test UFE_V1 interfaces, and therefore should not
		# depend on UFE_V2.  However, the test code relies on capability to
		# retrieve a USD prim from a UFE scene item, which in turn depends on
		# functionality that is only available in UFE_V2.  Therefore, run the
		# tests only if UFE_V2 is available.  PPT, 11-Oct-2019.
		#
		# MAYA-101385: until Maya switches back to UFE_V2, fail, so commented
		# out.  PPT, 11-Oct-2019.
		#
        # testDuplicateCmd.py
        # testMoveCmd.py
        testObject3d.py
        # testRotateCmd.py
        # testScaleCmd.py
        # testTransform3dTranslate.py
        testRename.py
    )
endif()

# copy ufe tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY test-samples)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeScripts)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeTestUtils)
mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY ufeTestPlugins)
mayaUsd_copyFiles(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                         FILES ${test_script_files})
mayaUsd_copyFiles(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                         FILES ${test_support_files})

# unit tests
set(path 
    "${CMAKE_INSTALL_PREFIX}/lib"
    "$ENV{PATH}"
)

set(pythonPath
    "${CMAKE_INSTALL_PREFIX}/lib/python"
    "$ENV{PYTHONPATH}"
)

set(mayaPluginPath
    "${CMAKE_INSTALL_PREFIX}/plugin/adsk/plugin"
    "${CMAKE_CURRENT_BINARY_DIR}/ufeTestPlugins"
)

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(mayaPluginPath)

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    add_test(
        NAME ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;from unittest import main; \
                                          import maya.standalone; \
                                          maya.standalone.initialize(name='python'); \
                                          import ${target};testProg=main(module=${target},exit=False);\
                                          maya.standalone.uninitialize(); \
                                          sys.exit(not testProg.result.wasSuccessful());
                                          "
    )
    set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT 
        "PYTHONPATH=${pythonPath}"
        "PATH=${path}"
        "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
        "PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
        "MAYA_NO_STANDALONE_ATEXIT=1"
    )
endforeach()
