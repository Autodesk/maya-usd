function(mayaUsd_get_unittest_target unittest_target unittest_basename)
    get_filename_component(unittest_name ${unittest_basename} NAME_WE)
    set(${unittest_target} "${unittest_name}" PARENT_SCOPE)
endfunction()

# This func gives explicit support for PYTHONPATH, PATH, and MAYA_PLUG_IN_PATH,
# PXR_PLUGINPATH_NAME, and MAYA_SCRIPT_PATH, because those would 'normally' be
# set up by mayaUSD.mod.
#
# Other vars may be set after this func is called, by just doing:
#   set_property(TEST ${TEST_NAME} APPEND PROPERTY ENVIRONMENT "MYENVVAR=${varvalue}")

function(mayaUsd_set_test_env test_name)
    set(all_path_vars
        PATH
        PYTHONPATH
        MAYA_PLUG_IN_PATH
        MAYA_SCRIPT_PATH
        PXR_PLUGINPATH_NAME
    )
    
    set(options
        AL
        PXR
        NO_ADSK
        GTEST
    )
    set(oneValueArgs
        MODULE
    )
    set(multiValueArgs
        ${all_path_vars}
    )
    
    cmake_parse_arguments(PREFIX
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    
    # Set up environ...

    # Set initial values for env vars from those passed in...
    foreach(pathvar ${all_path_vars})
        list(APPEND ${pathvar} ${PREFIX_${pathvar}})
    endforeach()

    if(IS_WINDOWS AND PREFIX_GTEST)
        list(APPEND PATH "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    
    # Emulate what mayaUSD.mod would do
    if(NOT PREFIX_NO_ADSK)
        list(APPEND PATH "${CMAKE_INSTALL_PREFIX}/lib")
        list(APPEND PYTHONPATH "${CMAKE_INSTALL_PREFIX}/lib/python")
        list(APPEND PXR_PLUGINPATH_NAME "${CMAKE_INSTALL_PREFIX}/lib/usd")
        list(APPEND MAYA_PLUG_IN_PATH "${CMAKE_INSTALL_PREFIX}/plugin/adsk/plugin")
    endif()
    
    if(PREFIX_PXR)
        list(APPEND PYTHONPATH "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/python")
        list(APPEND PATH "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
        list(APPEND MAYA_SCRIPT_PATH "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib/usd/usdMaya/resources")
        list(APPEND MAYA_PLUG_IN_PATH "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/plugin")
        list(APPEND PXR_PLUGINPATH_NAME "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/usd")
    endif()
    
    if(PREFIX_AL)
        list(APPEND PYTHONPATH "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python")
        list(APPEND PATH "${CMAKE_INSTALL_PREFIX}/plugin/al/lib")
        list(APPEND MAYA_PLUG_IN_PATH "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")
        list(APPEND PXR_PLUGINPATH_NAME "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/usd")
        list(APPEND PXR_PLUGINPATH_NAME "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")
    endif()
    
    # inherit PATH and PYTHONPATH from ENV to get USD entries
    # these should come last (esp PYTHONPATH, in case another module is overriding
    # with pkgutil) 
    list(APPEND PATH $ENV{PATH})
    list(APPEND PYTHONPATH $ENV{PYTHONPATH})
    
    # Finalize env vars
    foreach(pathvar ${all_path_vars})
        if(${pathvar})
            separate_argument_list(${pathvar})
            set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT 
                "${pathvar}=${${pathvar}}")
        endif()
    endforeach()
    
    # without "MAYA_NO_STANDALONE_ATEXIT=1", standalone.uninitialize() will
    # set exitcode to 0
    set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT 
        "MAYA_NO_STANDALONE_ATEXIT=1")
endfunction()

function(mayaUsd_add_py_test test_name pytest_code)
    set(options
        NO_STANDALONE_INIT
    )
    set(oneValueArgs
    )
    set(multiValueArgs
    )
    
    cmake_parse_arguments(PREFIX
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )
    
    if(NOT PREFIX_NO_STANDALONE_INIT)
        # first, indent pycode
        mayaUsd_indent(indented_pytest_code "${pytest_code}")
        # then wrap in try/finally, and call maya.standalone.[un]initialize()
        set(pytest_code "
import maya.standalone
maya.standalone.initialize(name='python')
try:
${indented_pytest_code}
finally:
    maya.standalone.uninitialize()
"
        )
    endif()
    
    add_test(
        NAME "${test_name}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${MAYA_PY_EXECUTABLE} -c "${pytest_code}"
    )
    
    mayaUsd_set_test_env("${test_name}" ${PREFIX_UNPARSED_ARGUMENTS})
endfunction()


function(mayaUsd_add_pymodule_test test_name)
    set(options
    )
    set(oneValueArgs
        MODULE
    )
    set(multiValueArgs
    )
    
    cmake_parse_arguments(PREFIX
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if(PREFIX_MODULE)
        set(module_name "${PREFIX_MODULE}")
    else()
        set(module_name "${test_name}")
    endif()
    set(pytest_code "
import sys
from unittest import main
import ${module_name}
main(module=${module_name})
")
    mayaUsd_add_py_test("${test_name}" "${pytest_code}" ${PREFIX_UNPARSED_ARGUMENTS})
endfunction()
