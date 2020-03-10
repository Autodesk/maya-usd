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

if (UFE_FOUND)
    add_subdirectory(ufe)
endif()

add_subdirectory(usd)
