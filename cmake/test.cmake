set(MAYA_USD_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(mayaUsd_get_unittest_target unittest_target unittest_basename)
    get_filename_component(unittest_name ${unittest_basename} NAME_WE)
    set(${unittest_target} "${unittest_name}" PARENT_SCOPE)
endfunction()

#
# mayaUsd_add_test( <test_name>
#                   {PYTHON_MODULE <python_module_name> |
#                    PYTHON_COMMAND <python_code> |
#                    PYTHON_SCRIPT <python_script_file> |
#                    COMMAND <cmd> [<cmdarg> ...] }
#                   [NO_STANDALONE_INIT]
#                   [INTERACTIVE]
#                   [ENV <varname>=<varvalue> ...])
#
#   PYTHON_MODULE      - Module to import and test with unittest.main.
#   PYTHON_COMMAND     - Python code to execute; should call sys.exit
#                        with an appropriate exitcode to indicate success
#                        or failure.
#   PYTHON_SCRIPT      - Python script file to execute; should exit with an
#                        appropriate exitcode to indicate success or failure.
#   WORKING_DIRECTORY  - Directory from which the test executable will be called.
#   COMMAND            - Command line to execute as a test
#   NO_STANDALONE_INIT - Only allowable with PYTHON_MODULE or
#                        PYTHON_COMMAND. With those modes, this
#                        command will generally add some boilerplate code
#                        to ensure that maya is initialized and exits
#                        correctly. Use this option to NOT add that code.
#   INTERACTIVE        - Only allowable with PYTHON_SCRIPT.
#                        The test is run using an interactive (non-standalone)
#                        session of Maya, including the UI.
#                        Tests run in this way should finish by calling Maya's
#                        quit command and returning an exit code of 0 for
#                        success or 1 for failure:
#                            cmds.quit(abort=True, exitCode=exitCode)
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
#                            XBMLANGPATH
#                            LD_LIBRARY_PATH
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
        "NO_STANDALONE_INIT;INTERACTIVE"                                # options
        "PYTHON_MODULE;PYTHON_COMMAND;PYTHON_SCRIPT;WORKING_DIRECTORY"  # one_value keywords
        "COMMAND;ENV"                                                   # multi_value keywords
        ${ARGN}
    )

    # check that they provided one and ONLY 1 of:
    #     PYTHON_MODULE / PYTHON_COMMAND / PYTHON_SCRIPT / COMMAND
    set(NUM_EXCLUSIVE_ITEMS 0)
    foreach(option_name PYTHON_MODULE PYTHON_COMMAND PYTHON_SCRIPT COMMAND)
        if(PREFIX_${option_name})
            math(EXPR NUM_EXCLUSIVE_ITEMS "${NUM_EXCLUSIVE_ITEMS} + 1")
        endif()
    endforeach()
    if(NOT NUM_EXCLUSIVE_ITEMS EQUAL 1)
        message(FATAL_ERROR "mayaUsd_add_test: must be called with exactly "
            "one of PYTHON_MODULE, PYTHON_COMMAND, PYTHON_SCRIPT, or COMMAND")
    endif()

    if(PREFIX_NO_STANDALONE_INIT AND NOT (PREFIX_PYTHON_MODULE
                                          OR PREFIX_PYTHON_COMMAND))
        message(FATAL_ERROR "mayaUsd_add_test: NO_STANDALONE_INIT may only be "
            "used with PYTHON_MODULE or PYTHON_COMMAND")
    endif()

    if(PREFIX_INTERACTIVE AND NOT PREFIX_PYTHON_SCRIPT)
        message(FATAL_ERROR "mayaUsd_add_test: INTERACTIVE may only be "
            "used with PYTHON_SCRIPT")
    endif()

    # set the working_dir
    if(PREFIX_WORKING_DIRECTORY)
        set(WORKING_DIR ${PREFIX_WORKING_DIRECTORY})
    else()
        set(WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    # --------------
    # 2) Create test
    # --------------

    set(PYTEST_CODE "")
    if(PREFIX_PYTHON_MODULE)
        set(MODULE_NAME "${PREFIX_PYTHON_MODULE}")
        set(PYTEST_CODE "
import sys
from unittest import main
import ${MODULE_NAME}
main(module=${MODULE_NAME})
")
    elseif(PREFIX_PYTHON_COMMAND)
        set(PYTEST_CODE "${PREFIX_PYTHON_COMMAND}")
    elseif(PREFIX_PYTHON_SCRIPT)
        if (PREFIX_INTERACTIVE)
            if(WIN32)
                set(QUOTE "'")
            else()
                set(QUOTE "\\\"")
            endif()
            set(MEL_PY_EXEC_COMMAND "python(\"\\n\
import os\\n\
import sys\\n\
import time\\n\
import traceback\\n\
file = ${QUOTE}${PREFIX_PYTHON_SCRIPT}${QUOTE}\\n\
if not os.path.isabs(file):\\n\
    file = os.path.join(${QUOTE}${CMAKE_CURRENT_SOURCE_DIR}${QUOTE}, file)\\n\
openMode = ${QUOTE}rb${QUOTE}\\n\
compileMode = ${QUOTE}exec${QUOTE}\\n\
globals = {${QUOTE}__file__${QUOTE}: file, ${QUOTE}__name__${QUOTE}: ${QUOTE}__main__${QUOTE}}\\n\
try:\\n\
    exec(compile(open(file, openMode).read(), file, compileMode), globals)\\n\
except Exception:\\n\
    sys.__stderr__.write(traceback.format_exc() + os.linesep)\\n\
    sys.__stderr__.flush()\\n\
    sys.__stdout__.flush()\\n\
    # sleep to give the output streams time to finish flushing - otherwise,\\n\
    # os._exit quits so hard + fast, flush may not happen!\\n\
    time.sleep(.1)\\n\
    os._exit(1)\\n\
\")")
            set(COMMAND_CALL ${MAYA_EXECUTABLE} -c ${MEL_PY_EXEC_COMMAND})
        else()
            set(SCRIPT ${CMAKE_BINARY_DIR}/test/Temporary/scripts/runner_${test_name}.py)
            FILE(WRITE ${SCRIPT} "${PREFIX_PYTHON_SCRIPT}")
            set(COMMAND_CALL ${MAYA_PY_EXECUTABLE} ${SCRIPT})
        endif()
    else()
        set(COMMAND_CALL ${PREFIX_COMMAND})
    endif()

    if(PYTEST_CODE)
        if(NOT PREFIX_NO_STANDALONE_INIT)
            # first, indent pycode
            mayaUsd_indent(indented_PYTEST_CODE "${PYTEST_CODE}")
            # then wrap in try/finally, and call maya.standalone.[un]initialize()
            set(PYTEST_CODE "
import maya.standalone
maya.standalone.initialize(name='python')
try:
${indented_PYTEST_CODE}
finally:
    maya.standalone.uninitialize()
"
            )
        endif()

        set(SCRIPT ${CMAKE_BINARY_DIR}/test/Temporary/scripts/runner_${test_name}.py)
        FILE(WRITE ${SCRIPT} "${PYTEST_CODE}")
        set(COMMAND_CALL ${MAYA_PY_EXECUTABLE} ${SCRIPT})
    endif()

    add_test(
        NAME "${test_name}"
        WORKING_DIRECTORY ${WORKING_DIR}
        COMMAND ${COMMAND_CALL}
    )

    # -----------------
    # 3) Set up environ
    # -----------------

    set(ALL_PATH_VARS
        PYTHONPATH
        MAYA_PLUG_IN_PATH
        MAYA_SCRIPT_PATH
        XBMLANGPATH
        ${PXR_OVERRIDE_PLUGINPATH_NAME}
        PXR_MTLX_STDLIB_SEARCH_PATHS
    )

    if(IS_WINDOWS)
        # Put path at the front of the list of env vars.
        list(INSERT ALL_PATH_VARS 0
            PATH
        )
    else()
        list(APPEND ALL_PATH_VARS
            LD_LIBRARY_PATH
        )
    endif()

    # Set initial empty values for all path vars
    foreach(pathvar ${ALL_PATH_VARS})
        set(MAYAUSD_VARNAME_${pathvar})
    endforeach()

    if(IS_WINDOWS)
        list(APPEND MAYAUSD_VARNAME_PATH "${CMAKE_INSTALL_PREFIX}/lib/gtest")
        list(APPEND MAYAUSD_VARNAME_PATH "${MAYA_LOCATION}/bin")
    endif()

    # NOTE - we prefix varnames with "MAYAUSD_VARNAME_" just to make collision
    # with some existing var less likely

    # Emulate what mayaUSD.mod would do

    # adsk
    list(APPEND MAYAUSD_VARNAME_PATH
         "${CMAKE_INSTALL_PREFIX}/lib")
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/lib/scripts")
    list(APPEND MAYAUSD_VARNAME_MAYA_SCRIPT_PATH
         "${CMAKE_INSTALL_PREFIX}/lib/scripts")
    if (IS_LINUX)
        # On Linux the paths in XBMLANGPATH need a /%B at the end.
        list(APPEND MAYAUSD_VARNAME_XBMLANGPATH
             "${CMAKE_INSTALL_PREFIX}/lib/icons/%B")
    else()
        list(APPEND MAYAUSD_VARNAME_XBMLANGPATH
             "${CMAKE_INSTALL_PREFIX}/lib/icons")
    endif()
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/lib/python")
    list(APPEND MAYAUSD_VARNAME_${PXR_OVERRIDE_PLUGINPATH_NAME}
         "${CMAKE_INSTALL_PREFIX}/lib/usd")
    list(APPEND MAYAUSD_VARNAME_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/adsk/plugin")
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/plugin/adsk/scripts")
    list(APPEND MAYAUSD_VARNAME_MAYA_SCRIPT_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/adsk/scripts")
    list(APPEND MAYAUSD_VARNAME_PXR_MTLX_STDLIB_SEARCH_PATHS
         "${PXR_USD_LOCATION}/libraries")
    list(APPEND MAYAUSD_VARNAME_PXR_MTLX_STDLIB_SEARCH_PATHS
         "${CMAKE_INSTALL_PREFIX}/libraries")

    # pxr
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/python")
    list(APPEND MAYAUSD_VARNAME_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    list(APPEND MAYAUSD_VARNAME_MAYA_SCRIPT_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib/usd/usdMaya/resources")
    list(APPEND MAYAUSD_VARNAME_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/plugin")
    list(APPEND MAYAUSD_VARNAME_${PXR_OVERRIDE_PLUGINPATH_NAME}
         "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib/usd")

    # al
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python")
    list(APPEND MAYAUSD_VARNAME_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib")
    list(APPEND MAYAUSD_VARNAME_MAYA_PLUG_IN_PATH
         "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")
    list(APPEND MAYAUSD_VARNAME_${PXR_OVERRIDE_PLUGINPATH_NAME}
         "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/usd")
    list(APPEND MAYAUSD_VARNAME_${PXR_OVERRIDE_PLUGINPATH_NAME}
         "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin")

    if(IS_WINDOWS AND DEFINED ENV{PYTHONHOME})
        # If the environment contains a PYTHONHOME, also set the path to
        # that folder so that we can find the python DLLs.
        list(APPEND MAYAUSD_VARNAME_PATH $ENV{PYTHONHOME})
    endif()

    # Adjust PYTHONPATH to include the path to our test utilities.
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH "${MAYA_USD_DIR}/test/testUtils")

    # Adjust PYTHONPATH to include the path to our test.
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH "${CMAKE_CURRENT_SOURCE_DIR}")

    # Adjust PATH and PYTHONPATH to include USD.
    # These should come last (esp PYTHONPATH, in case another module is overriding
    # with pkgutil)
   if (DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        set(USD_INSTALL_LOCATION "${CMAKE_INSTALL_PREFIX}/${MAYAUSD_TO_USD_RELATIVE_PATH}")
    else()
        set(USD_INSTALL_LOCATION ${PXR_USD_LOCATION})
    endif()
    # Inherit any existing PYTHONPATH, but keep it at the end.
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH
        "${USD_INSTALL_LOCATION}/lib/python")
    if(IS_WINDOWS)
        list(APPEND MAYAUSD_VARNAME_PATH
            "${USD_INSTALL_LOCATION}/bin")
        list(APPEND MAYAUSD_VARNAME_PATH
            "${USD_INSTALL_LOCATION}/lib")
    endif()

    # NOTE: this should come after any setting of PATH/PYTHONPATH so
    #       that our entries will come first.
    # Inherit any existing PATH/PYTHONPATH, but keep it at the end.
    # This is needed (especially for PATH) because we will overwrite
    # both with the values from our list and we need to keep any
    # system entries.
    list(APPEND MAYAUSD_VARNAME_PATH $ENV{PATH})
    list(APPEND MAYAUSD_VARNAME_PYTHONPATH $ENV{PYTHONPATH})

    # convert the internally-processed envs from cmake list
    foreach(pathvar ${ALL_PATH_VARS})
        separate_argument_list(MAYAUSD_VARNAME_${pathvar})
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
        if("${env_name}" IN_LIST ALL_PATH_VARS)
            if(IS_WINDOWS)
                set(MAYAUSD_VARNAME_${env_name}
                    "${env_value}\;${MAYAUSD_VARNAME_${env_name}}")
            else()
                set(MAYAUSD_VARNAME_${env_name}
                    "${env_value}:${MAYAUSD_VARNAME_${env_name}}")
            endif()
        else()
            set("MAYAUSD_VARNAME_${env_name}" ${env_value})
            list(APPEND ALL_PATH_VARS "${env_name}")
        endif()
    endforeach()

    # Unset any MAYA_MODULE_PATH as we set all the individual variables
    # so we don't want to conflict with a MayaUsd module.
    set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT "MAYA_MODULE_PATH=")

    # set all env vars
    foreach(pathvar ${ALL_PATH_VARS})
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "${pathvar}=${MAYAUSD_VARNAME_${pathvar}}")
    endforeach()
    
    # Set a temporary folder path for the TMP,TEMP and MAYA_APP_DIR in which the
    # maya profile will be created.
    # Note: replace bad chars in test_name with _.
    string(REGEX REPLACE "[:<>\|]" "_" SANITIZED_TEST_NAME ${test_name})
    set(MAYA_APP_TEMP_DIR "${CMAKE_BINARY_DIR}/test/Temporary/${SANITIZED_TEST_NAME}")
    # Note: ${WORKING_DIR} can point to the source folder, so don't use it
    #       in any env var that will write files (such as MAYA_APP_DIR).
    set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
        "TMP=${MAYA_APP_TEMP_DIR}"
        "TEMP=${MAYA_APP_TEMP_DIR}"
        "MAYA_APP_DIR=${MAYA_APP_TEMP_DIR}")
    file(MAKE_DIRECTORY ${MAYA_APP_TEMP_DIR})

    # Set the Python major version in MAYA_PYTHON_VERSION. Maya 2020 and
    # earlier that are Python 2 only will simply ignore it.
    # without "MAYA_NO_STANDALONE_ATEXIT=1", standalone.uninitialize() will
    # set exitcode to 0
    # MAYA_DISABLE_CIP=1  Avoid fatal crash on start-up.
    # MAYA_DISABLE_CER=1  Customer Error Reporting.
    set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
        "MAYA_PYTHON_VERSION=${MAYA_PY_VERSION}"
        "MAYA_NO_STANDALONE_ATEXIT=1"
        "MAYA_DEBUG_ENABLE_CRASH_REPORTING=1"
        "MAYA_DEBUG_NO_SAVE_ON_CRASH=1"
        "MAYA_NO_MORE_ASSERT=1"
        "MAYA_DISABLE_CIP=1"
        "MAYA_DISABLE_CER=1")

    if(IS_MACOSX)
        # Necessary for tests like DiffCore to find python
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "DYLD_LIBRARY_PATH=${MAYA_LOCATION}/MacOS:$ENV{DYLD_LIBRARY_PATH}")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "DYLD_FRAMEWORK_PATH=${MAYA_LOCATION}/Maya.app/Contents/Frameworks")
    endif()

    if (PREFIX_INTERACTIVE)
        # Add the "interactive" label to all tests that launch the Maya UI.
        # This allows bypassing them by using the --label-exclude/-LE option to
        # ctest. This is useful when running tests in a headless configuration.
        set_property(TEST "${test_name}" APPEND PROPERTY LABELS interactive)

        # When running via remote desktop this env var is needed for Maya
        # to function correctly. Has no effect when not running remote.
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "MAYA_ALLOW_OPENGL_REMOTE_SESSION=1")

        # Don't want popup when color management fails.
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "MAYA_CM_DISABLE_ERROR_POPUPS=1")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "MAYA_COLOR_MGT_NO_LOGGING=1")
            
    else()
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT
            "MAYA_IGNORE_DIALOGS=1")
    endif()
endfunction()
