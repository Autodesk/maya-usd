function(mayaUsd_get_unittest_target unittest_target unittest_basename)
    get_filename_component(unittest_name ${unittest_basename} NAME_WE)
    set(${unittest_target} "${unittest_name}" PARENT_SCOPE)
endfunction()

#
# mayaUsd_add_test( <test_name>
#                   {PYTHON_MODULE <python_module_name> |
#                    PYTHON_COMMAND <python_code> |
#                    COMMAND <cmd> [<cmdarg> ...] }
#                   [NO_STANDALONE_INIT]
#                   [ENV <varname>=<varvalue> ...])
#
#   PYTHON_MODULE      - Module to import and test with unittest.main.
#   PYTHON_COMMAND     - Python code to execute; should call sys.exit
#                        with an appropriate exitcode to indicate success
#                        or failure.
#   COMMAND            - Command line to execute as a test
#   NO_STANDALONE_INIT - Only allowable with PYTHON_MODULE or
#                        PYTHON_COMMAND. With those modes, this
#                        command will generally add some boilerplate code
#                        to ensure that maya is initialized and exits
#                        correctly. Use this option to NOT add that code.
#   ENV                - Set or append the indicated environment variables;
#                        Since mayaUsd_add_test internally makes changes to
#                        some environment variables, if a value is given
#                        for these variables, it is appended; all other
#                        variables are set exactly as given. The variables
#                        that mayaUsd_add_test manages (and will append) are:
#                            PATH
#                            PYTHONPATH
#                            MAYA_PLUG_IN_PATH
#                            MAYA_SCRIPT_PATH
#                            PXR_PLUGINPATH_NAME
#                        Note that the format of these name/value pairs should
#                        be the same as that used with
#                        `set_property(TEST test_name APPEND PROPERTY ENVIRONMENT ...)`
#                        That means that if the passed in env var is a "list", it
#                        must already be separated by platform-appropriate
#                        path-separators, escaped if needed - ie, ":" on
#                        Linux/MacOS, and "\;" on Windows. Use
#                        separate_argument_list before passing to this func
#                        if you start with a cmake-style list.
#
function(mayaUsd_add_test test_name)
    # -----------------
    # 1) Arg processing
    # -----------------

    cmake_parse_arguments(PREFIX
        "NO_STANDALONE_INIT"            # options
        "PYTHON_MODULE;PYTHON_COMMAND"  # one_value keywords
        "COMMAND;ENV"                   # multi_value keywords
        ${ARGN}
    )

    # check that they provided one and ONLY 1 of:
    #     PYTHON_MODULE / PYTHON_COMMAND / COMMAND
    set(num_exclusive_items 0)
    foreach(option_name PYTHON_MODULE PYTHON_COMMAND COMMAND)
        if(PREFIX_${option_name})
            math(EXPR num_exclusive_items "${num_exclusive_items} + 1")
        endif()
    endforeach()
    if(NOT num_exclusive_items EQUAL 1)
        message(FATAL_ERROR "mayaUsd_add_test: must be called with exactly "
            "one of PYTHON_MODULE, PYTHON_COMMAND, or COMMAND")
    endif()

    if(PREFIX_NO_STANDALONE_INIT AND NOT (PREFIX_PYTHON_MODULE
                                          OR PREFIX_PYTHON_COMMAND))
        message(FATAL_ERROR "mayaUsd_add_test: NO_STANDALONE_INIT may only be "
            "used with PYTHON_MODULE or PYTHON_COMMAND")
    endif()

    # --------------
    # 2) Create test
    # --------------

    set(pytest_code "")
    if(PREFIX_PYTHON_MODULE)
        set(module_name "${PREFIX_PYTHON_MODULE}")
        set(pytest_code "
import sys
from unittest import main
import ${module_name}
main(module=${module_name})
")
    elseif(PREFIX_PYTHON_COMMAND)
        set(pytest_code "${PREFIX_PYTHON_COMMAND}")
    else()
        set(command ${PREFIX_COMMAND})
    endif()

    if(pytest_code)
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

        string(REPLACE ";" "\;" pytest_code "${pytest_code}")
        set(command ${MAYA_PY_EXECUTABLE} -c "${pytest_code}")

    endif()

    add_test(
        NAME "${test_name}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${command}
    )

    # -----------------
    # 3) Set up environ
    # -----------------

    set(all_path_vars
        PATH
        PYTHONPATH
        MAYA_PLUG_IN_PATH
        MAYA_SCRIPT_PATH
        PXR_PLUGINPATH_NAME
    )

    # Set initial empty values for all path vars
    foreach(pathvar ${all_path_vars})
        set(mayaUsd_varname_${pathvar})
    endforeach()

    if(IS_WINDOWS)
        list(APPEND mayaUsd_varname_PATH "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()

    # NOTE - we prefix varnames with "mayaUsd_varname_" just to make collision
    # with some existing var less likely

    # Emulate what mayaUSD.mod would do

    # adsk
    list(APPEND mayaUsd_varname_PATH
         "${CMAKE_INSTALL_PREFIX}/lib")
    list(APPEND mayaUsd_varname_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/lib/python")
    list(APPEND mayaUsd_varname_PXR_PLUGINPATH_NAME
         "${CMAKE_INSTALL_PREFIX}/lib/usd")
    list(APPEND mayaUsd_varname_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/adsk/plugin")

    # pxr
    list(APPEND mayaUsd_varname_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/python")
    list(APPEND mayaUsd_varname_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    list(APPEND mayaUsd_varname_MAYA_SCRIPT_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib/usd/usdMaya/resources")
    list(APPEND mayaUsd_varname_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/plugin")
    list(APPEND mayaUsd_varname_PXR_PLUGINPATH_NAME
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/usd")

    # al
    list(APPEND mayaUsd_varname_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python")
    list(APPEND mayaUsd_varname_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib")
    list(APPEND mayaUsd_varname_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")
    list(APPEND mayaUsd_varname_PXR_PLUGINPATH_NAME
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/usd")
    list(APPEND mayaUsd_varname_PXR_PLUGINPATH_NAME
         "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")

    # inherit PATH and PYTHONPATH from ENV to get USD entries
    # these should come last (esp PYTHONPATH, in case another module is overriding
    # with pkgutil)
    list(APPEND mayaUsd_varname_PATH $ENV{PATH})
    list(APPEND mayaUsd_varname_PYTHONPATH $ENV{PYTHONPATH})

    # convert the internally-processed envs from cmake list
    foreach(pathvar ${all_path_vars})
        separate_argument_list(mayaUsd_varname_${pathvar})
    endforeach()

    # prepend the passed-in ENV values - assume these are already
    # separated + escaped
    foreach(name_value_pair ${PREFIX_ENV})
        mayaUsd_split_head_tail("${name_value_pair}" "=" env_name env_value)
        if(NOT env_name)
            message(FATAL_ERROR "poorly formatted NAME=VALUE pair - name "
                "missing: ${name_value_pair}")
        endif()

        # now either prepend to existing list, or create new
        if("${env_name}" IN_LIST all_path_vars)
            if(IS_WINDOWS)
                set(mayaUsd_varname_${env_name}
                    "${env_value}\;${mayaUsd_varname_${env_name}}")
            else()
                set(mayaUsd_varname_${env_name}
                    "${env_value}:${mayaUsd_varname_${env_name}}")
            endif()
        else()
            set("mayaUsd_varname_${env_name}" ${env_value})
            list(APPEND all_path_vars "${env_name}")
        endif()
    endforeach()

    # set all env vars
    foreach(pathvar ${all_path_vars})
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "${pathvar}=${mayaUsd_varname_${pathvar}}")
    endforeach()

    # without "MAYA_NO_STANDALONE_ATEXIT=1", standalone.uninitialize() will
    # set exitcode to 0
    set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
        "MAYA_NO_STANDALONE_ATEXIT=1")
endfunction()
