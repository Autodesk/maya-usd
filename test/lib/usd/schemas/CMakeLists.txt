set(TARGET_NAME MAYAUSD_SCHEMA_TEST)

# Unit test scripts.
set(test_script_files
    testMayaUsdSchemasPythonImport.py
    testMayaUsdSchemasMayaReference.py
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

set(pythonPath
    "${CMAKE_INSTALL_PREFIX}/lib/python"
    "$ENV{PYTHONPATH}"
)

set(pxrPluginPath 
    "${CMAKE_INSTALL_PREFIX}/lib/usd"
)

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(pxrPluginPath)

foreach(script ${test_script_files})
    mayaUsd_get_unittest_target(target ${script})
    add_test(
        NAME ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${MAYA_PY_EXECUTABLE} -c "from unittest import main; \
                                          import maya.standalone; \
                                          maya.standalone.initialize(name='python'); \
                                          import ${target}; \
                                          main(module=${target}); \
                                          maya.standalone.uninitialize()"
    )
    set_property(TEST ${target} APPEND PROPERTY ENVIRONMENT
        "PATH=${path}"
        "PYTHONPATH=${pythonPath}"
        "PXR_PLUGINPATH_NAME=${pxrPluginPath}"
        "MAYA_NO_STANDALONE_ATEXIT=1"
    )
endforeach()
