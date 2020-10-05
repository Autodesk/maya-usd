#
# Copyright 2016 Pixar
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
include(Private)

function(pxr_library NAME)
    set(options
        DISABLE_PRECOMPILED_HEADERS
        MAYA_PLUGIN
    )
    set(oneValueArgs
        TYPE
        PRECOMPILED_HEADER_NAME
    )
    set(multiValueArgs
        PUBLIC_CLASSES
        PUBLIC_HEADERS
        PRIVATE_CLASSES
        PRIVATE_HEADERS
        CPPFILES
        LIBRARIES
        INCLUDE_DIRS
        RESOURCE_FILES
        PYTHON_PUBLIC_CLASSES
        PYTHON_PRIVATE_CLASSES
        PYTHON_PUBLIC_HEADERS
        PYTHON_PRIVATE_HEADERS
        PYTHON_CPPFILES
        PYMODULE_CPPFILES
        PYMODULE_FILES
        PYSIDE_UI_FILES
    )

    cmake_parse_arguments(args
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Merge the python specific categories with the more general before setting
    # up compilation.
    if(args_PYTHON_PUBLIC_CLASSES)
        list(APPEND args_PUBLIC_CLASSES ${args_PYTHON_PUBLIC_CLASSES})
    endif()
    if(args_PYTHON_PUBLIC_HEADERS)
        list(APPEND args_PUBLIC_HEADERS ${args_PYTHON_PUBLIC_HEADERS})
    endif()
    if(args_PYTHON_PRIVATE_CLASSES)
        list(APPEND args_PRIVATE_CLASSES ${args_PYTHON_PRIVATE_CLASSES})
    endif()
    if(args_PYTHON_PRIVATE_HEADERS)
        list(APPEND args_PRIVATE_HEADERS ${args_PYTHON_PRIVATE_HEADERS})
    endif()
    if(args_PYTHON_CPPFILES)
        list(APPEND args_CPPFILES ${args_PYTHON_CPPFILES})
    endif()

    # Collect libraries.
    if(NOT args_TYPE STREQUAL "PLUGIN")
        get_property(help CACHE PXR_ALL_LIBS PROPERTY HELPSTRING)
        list(APPEND PXR_ALL_LIBS ${NAME})
        set(PXR_ALL_LIBS "${PXR_ALL_LIBS}" CACHE INTERNAL "${help}")
        if(args_TYPE STREQUAL "STATIC")
            # Note if this library is explicitly STATIC.
            get_property(help CACHE PXR_STATIC_LIBS PROPERTY HELPSTRING)
            list(APPEND PXR_STATIC_LIBS ${NAME})
            set(PXR_STATIC_LIBS "${PXR_STATIC_LIBS}" CACHE INTERNAL "${help}")
        endif()
    endif()

    # Expand classes into filenames.
    _classes(${NAME} ${args_PRIVATE_CLASSES} PRIVATE)
    _classes(${NAME} ${args_PUBLIC_CLASSES} PUBLIC)

    # Custom tweaks.
    if(args_TYPE STREQUAL "PLUGIN")
        # We can't build plugins if we're not building shared libraries.
        if(NOT TARGET shared_libs)
            message(STATUS "Skipping plugin ${NAME}, shared libraries required")
            return()
        endif()

        set(prefix "")
        set(suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})

        # Maya plugins require the .mll suffix on Windows and .bundle on OSX.
        if(args_MAYA_PLUGIN)
            if (WIN32)
                set(suffix ".mll")
            elseif(IS_MACOSX)
                set(suffix ".bundle")
            endif()
        endif()
    else()
        # If the caller didn't specify the library type then choose the
        # type now.
        if("x${args_TYPE}" STREQUAL "x")
            if(BUILD_SHARED_LIBS)
                set(args_TYPE "SHARED")
            else()
                set(args_TYPE "STATIC")
            endif()
        endif()

        set(prefix "${CMAKE_SHARED_LIBRARY_PREFIX}")
        if(args_TYPE STREQUAL "STATIC")
            set(suffix ${CMAKE_STATIC_LIBRARY_SUFFIX})
        else()
            set(suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
        endif()
    endif()

    set(pch "ON")
    if(args_DISABLE_PRECOMPILED_HEADERS)
        set(pch "OFF")
    endif()

    _pxr_library(${NAME}
        TYPE "${args_TYPE}"
        PREFIX "${prefix}"
        SUFFIX "${suffix}"
        SUBDIR "${subdir}"
        CPPFILES "${args_CPPFILES};${${NAME}_CPPFILES}"
        PUBLIC_HEADERS "${args_PUBLIC_HEADERS};${${NAME}_PUBLIC_HEADERS}"
        PRIVATE_HEADERS "${args_PRIVATE_HEADERS};${${NAME}_PRIVATE_HEADERS}"
        LIBRARIES "${args_LIBRARIES}"
        INCLUDE_DIRS "${args_INCLUDE_DIRS}"
        RESOURCE_FILES "${args_RESOURCE_FILES}"
        PRECOMPILED_HEADERS "${pch}"
        PRECOMPILED_HEADER_NAME "${args_PRECOMPILED_HEADER_NAME}"
        LIB_INSTALL_PREFIX_RESULT libInstallPrefix
    )

    if(args_PYMODULE_CPPFILES OR args_PYMODULE_FILES OR args_PYSIDE_UI_FILES)
        _pxr_python_module(
            ${NAME}
            WRAPPED_LIB_INSTALL_PREFIX "${INSTALL_DIR_SUFFIX}/${libInstallPrefix}"
            PYTHON_FILES ${args_PYMODULE_FILES}
            PYSIDE_UI_FILES ${args_PYSIDE_UI_FILES}
            CPPFILES ${args_PYMODULE_CPPFILES}
            INCLUDE_DIRS ${args_INCLUDE_DIRS}
            PRECOMPILED_HEADERS ${pch}
            PRECOMPILED_HEADER_NAME ${args_PRECOMPILED_HEADER_NAME}
        )
    endif()
endfunction()

macro(pxr_shared_library NAME)
    pxr_library(${NAME} TYPE "SHARED" ${ARGN})
endmacro(pxr_shared_library)

macro(pxr_static_library NAME)
    pxr_library(${NAME} TYPE "STATIC" ${ARGN})
endmacro(pxr_static_library)

macro(pxr_plugin NAME)
    pxr_library(${NAME} TYPE "PLUGIN" ${ARGN})
endmacro(pxr_plugin)

function(pxr_setup_python)
    # Install a pxr __init__.py in order to have Python 
    # see UsdMaya module inside pxr subdirectory 
    _get_install_dir(lib/python/pxr installPrefix)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/__init__.py"
    "try:\n  __import__('pkg_resources').declare_namespace(__name__)\nexcept:\n  from pkgutil import extend_path\n  __path__ = extend_path(__path__, __name__)\n")
	execute_process(COMMAND ${Python_EXECUTABLE} -m compileall ${CMAKE_CURRENT_BINARY_DIR}/__init__.py)
    install(
        FILES
          ${CMAKE_CURRENT_BINARY_DIR}/__init__.py
        DESTINATION
          ${INSTALL_DIR_SUFFIX}/${installPrefix}
    )
endfunction() # pxr_setup_python

function (pxr_create_test_module MODULE_NAME)
    # If we can't build Python modules then do nothing.
    if(NOT TARGET python)
        return()
    endif()

    if (NOT BUILD_TESTS)
        return()
    endif()

    cmake_parse_arguments(tm "" "INSTALL_PREFIX;SOURCE_DIR" "" ${ARGN})

    if (NOT tm_SOURCE_DIR)
        set(tm_SOURCE_DIR testenv)
    endif()

    # Look specifically for an __init__.py and a plugInfo.json prefixed by the
    # module name. These will be installed without the module prefix.
    set(initPyFile ${tm_SOURCE_DIR}/${MODULE_NAME}__init__.py)
    set(plugInfoFile ${tm_SOURCE_DIR}/${MODULE_NAME}_plugInfo.json)

    # XXX -- We shouldn't have to install to run tests.
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${initPyFile}")
        install(
            FILES 
                ${initPyFile}
            RENAME 
                __init__.py
            DESTINATION 
                ${INSTALL_DIR_SUFFIX}/tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
        )
    endif()
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${plugInfoFile}")
        install(
            FILES 
                ${plugInfoFile}
            RENAME 
                plugInfo.json
            DESTINATION 
                ${INSTALL_DIR_SUFFIX}/tests/${tm_INSTALL_PREFIX}/lib/python/${MODULE_NAME}
        )
    endif()
endfunction() # pxr_create_test_module

function(pxr_build_test_shared_lib LIBRARY_NAME)
    if (BUILD_TESTS)
        cmake_parse_arguments(bt
            ""
            "INSTALL_PREFIX;SOURCE_DIR"
            "LIBRARIES;CPPFILES"
            ${ARGN}
        )
        
        add_library(${LIBRARY_NAME}
            SHARED
            ${bt_CPPFILES}
        )
        _pxr_target_link_libraries(${LIBRARY_NAME}
            ${bt_LIBRARIES}
        )
        _get_folder("tests/lib" folder)
        set_target_properties(${LIBRARY_NAME}
            PROPERTIES 
                FOLDER "${folder}"
        )

        # compiler configuration
        mayaUsd_compile_config(${LIBRARY_NAME})

        # Find libraries under the install prefix, which has the core USD
        # libraries.
        mayaUsd_init_rpath(rpath "tests/lib")
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
        mayaUsd_install_rpath(rpath ${LIBRARY_NAME})

        if (NOT bt_SOURCE_DIR)
            set(bt_SOURCE_DIR testenv)
        endif()
        set(testPlugInfoSrcPath ${bt_SOURCE_DIR}/${LIBRARY_NAME}_plugInfo.json)

        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${testPlugInfoSrcPath}")
            set(TEST_PLUG_INFO_RESOURCE_PATH "Resources")
            set(TEST_PLUG_INFO_ROOT "..")
            set(LIBRARY_FILE "${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

            set(testPlugInfoLibDir "tests/${bt_INSTALL_PREFIX}/lib/${LIBRARY_NAME}")
            set(testPlugInfoResourceDir "${testPlugInfoLibDir}/${TEST_PLUG_INFO_RESOURCE_PATH}")
            set(testPlugInfoPath "${CMAKE_BINARY_DIR}/${testPlugInfoResourceDir}/plugInfo.json")

            file(RELATIVE_PATH 
                TEST_PLUG_INFO_LIBRARY_PATH
                "${CMAKE_INSTALL_PREFIX}/${testPlugInfoLibDir}"
                "${CMAKE_INSTALL_PREFIX}/tests/lib/${LIBRARY_FILE}")

            configure_file("${testPlugInfoSrcPath}" "${testPlugInfoPath}")
            # XXX -- We shouldn't have to install to run tests.
            install(
                FILES ${testPlugInfoPath}
                DESTINATION ${INSTALL_DIR_SUFFIX}/${testPlugInfoResourceDir})
        endif()

        # We always want this test to build after the package it's under, even if
        # it doesn't link directly. This ensures that this test is able to include
        # headers from its parent package.
        add_dependencies(${LIBRARY_NAME} ${PXR_PACKAGE})

        # Test libraries can include the private headers of their parent PXR_PACKAGE
        # library
        target_include_directories(${LIBRARY_NAME}
            PRIVATE $<TARGET_PROPERTY:${PXR_PACKAGE},INCLUDE_DIRECTORIES>
        )

        # XXX -- We shouldn't have to install to run tests.
        install(
            TARGETS ${LIBRARY_NAME}
            LIBRARY DESTINATION "${INSTALL_DIR_SUFFIX}/tests/lib"
            ARCHIVE DESTINATION "${INSTALL_DIR_SUFFIX}/tests/lib"
            RUNTIME DESTINATION "${INSTALL_DIR_SUFFIX}/tests/lib"
        )
    endif()
endfunction() # pxr_build_test_shared_lib

function(pxr_build_test TEST_NAME)
    if (BUILD_TESTS)
        cmake_parse_arguments(bt
            "" ""
            "LIBRARIES;CPPFILES"
            ${ARGN}
        )

        add_executable(${TEST_NAME}
            ${bt_CPPFILES}
        )

        # Turn PIC ON otherwise ArchGetAddressInfo() on Linux may yield
        # unexpected results.
        _get_folder("tests/bin" folder)
        set_target_properties(${TEST_NAME}
            PROPERTIES 
                FOLDER "${folder}"
            	POSITION_INDEPENDENT_CODE ON
        )
        target_include_directories(${TEST_NAME}
            PRIVATE $<TARGET_PROPERTY:${PXR_PACKAGE},INCLUDE_DIRECTORIES>
        )
        _pxr_target_link_libraries(${TEST_NAME}
            ${bt_LIBRARIES}
        )

        # Find libraries under the install prefix, which has the core USD
        # libraries.
        mayaUsd_init_rpath(rpath "tests")
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
        mayaUsd_install_rpath(rpath ${TEST_NAME})

        # XXX -- We shouldn't have to install to run tests.
        install(TARGETS ${TEST_NAME}
            RUNTIME DESTINATION "${INSTALL_DIR_SUFFIX}/tests"
        )
    endif()
endfunction() # pxr_build_test

function(pxr_test_scripts)
    # If we can't build Python modules then do nothing.
    if(NOT TARGET python)
        return()
    endif()

    if (NOT BUILD_TESTS)
        return()
    endif()

    foreach(file ${ARGN})
        get_filename_component(destFile ${file} NAME_WE)
        # XXX -- We shouldn't have to install to run tests.
        install(
            PROGRAMS ${file}
            DESTINATION ${INSTALL_DIR_SUFFIX}/tests
            RENAME ${destFile}
        )
    endforeach()
endfunction() # pxr_test_scripts

function(pxr_install_test_dir)
    if (BUILD_TESTS)
        cmake_parse_arguments(bt
            "" 
            "SRC;DEST"
            ""
            ${ARGN}
        )

        # XXX -- We shouldn't have to install to run tests.
        install(
            DIRECTORY ${bt_SRC}/
            DESTINATION ${INSTALL_DIR_SUFFIX}/tests/ctest/${bt_DEST}
        )
    endif()
endfunction() # pxr_install_test_dir

function(pxr_register_test TEST_NAME)
    if (BUILD_TESTS)
        cmake_parse_arguments(bt
            "RUN_SERIAL;PYTHON;REQUIRES_SHARED_LIBS;REQUIRES_PYTHON_MODULES" 
            "CUSTOM_PYTHON;COMMAND;STDOUT_REDIRECT;STDERR_REDIRECT;DIFF_COMPARE;POST_COMMAND;POST_COMMAND_STDOUT_REDIRECT;POST_COMMAND_STDERR_REDIRECT;PRE_COMMAND;PRE_COMMAND_STDOUT_REDIRECT;PRE_COMMAND_STDERR_REDIRECT;FILES_EXIST;FILES_DONT_EXIST;CLEAN_OUTPUT;EXPECTED_RETURN_CODE;TESTENV"
            "ENV;PRE_PATH;POST_PATH"
            ${ARGN}
        )

        # Discard tests that required shared libraries.
        if(NOT TARGET shared_libs)
            # Explicit requirement.  This is for C++ tests that dynamically
            # load libraries linked against USD code.  These tests will have
            # multiple copies of symbols and will likely re-execute
            # ARCH_CONSTRUCTOR and registration functions, which will almost
            # certainly cause problems.
            if(bt_REQUIRES_SHARED_LIBS)
                message(STATUS "Skipping test ${TEST_NAME}, shared libraries required")
                return()
            endif()
        endif()

        if(NOT TARGET python)
            # Implicit requirement.  Python modules require shared USD
            # libraries.  If the test runs python it's certainly going
            # to load USD modules.  If the test uses C++ to load USD
            # modules it tells us via REQUIRES_PYTHON_MODULES.
            if(bt_PYTHON OR bt_CUSTOM_PYTHON OR bt_REQUIRES_PYTHON_MODULES)
                message(STATUS "Skipping test ${TEST_NAME}, Python modules required")
                return()
            endif()
        endif()

        # This harness is a filter which allows us to manipulate the test run, 
        # e.g. by changing the environment, changing the expected return code, etc.
        set(testWrapperCmd ${PROJECT_SOURCE_DIR}/${INSTALL_DIR_SUFFIX}/cmake/macros/testWrapper.py --verbose)

        if (bt_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --stdout-redirect=${bt_STDOUT_REDIRECT})
        endif()

        if (bt_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --stderr-redirect=${bt_STDERR_REDIRECT})
        endif()

        if (bt_PRE_COMMAND_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --pre-command-stdout-redirect=${bt_PRE_COMMAND_STDOUT_REDIRECT})
        endif()

        if (bt_PRE_COMMAND_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --pre-command-stderr-redirect=${bt_PRE_COMMAND_STDERR_REDIRECT})
        endif()

        if (bt_POST_COMMAND_STDOUT_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --post-command-stdout-redirect=${bt_POST_COMMAND_STDOUT_REDIRECT})
        endif()

        if (bt_POST_COMMAND_STDERR_REDIRECT)
            set(testWrapperCmd ${testWrapperCmd} --post-command-stderr-redirect=${bt_POST_COMMAND_STDERR_REDIRECT})
        endif()

        # Not all tests will have testenvs, but if they do let the wrapper know so
        # it can copy the testenv contents into the run directory. By default,
        # assume the testenv has the same name as the test but allow it to be
        # overridden by specifying TESTENV.
        if (bt_TESTENV)
            set(testenvDir ${PXR_INSTALL_PREFIX}/tests/ctest/${bt_TESTENV})
        else()
            set(testenvDir ${PXR_INSTALL_PREFIX}/tests/ctest/${TEST_NAME})
        endif()

        set(testWrapperCmd ${testWrapperCmd} --testenv-dir=${testenvDir})

        if (bt_DIFF_COMPARE)
            set(testWrapperCmd ${testWrapperCmd} --diff-compare=${bt_DIFF_COMPARE})

            # For now the baseline directory is assumed by convention from the test
            # name. There may eventually be cases where we'd want to specify it by
            # an argument though.
            set(baselineDir ${testenvDir}/baseline)
            set(testWrapperCmd ${testWrapperCmd} --baseline-dir=${baselineDir})
        endif()

        if (bt_CLEAN_OUTPUT)
            set(testWrapperCmd ${testWrapperCmd} --clean-output-paths=${bt_CLEAN_OUTPUT})
        endif()

        if (bt_FILES_EXIST)
            set(testWrapperCmd ${testWrapperCmd} --files-exist=${bt_FILES_EXIST})
        endif()

        if (bt_FILES_DONT_EXIST)
            set(testWrapperCmd ${testWrapperCmd} --files-dont-exist=${bt_FILES_DONT_EXIST})
        endif()

        if (bt_PRE_COMMAND)
            set(testWrapperCmd ${testWrapperCmd} --pre-command=${bt_PRE_COMMAND})
        endif()

        if (bt_POST_COMMAND)
            set(testWrapperCmd ${testWrapperCmd} --post-command=${bt_POST_COMMAND})
        endif()

        if (bt_EXPECTED_RETURN_CODE)
            set(testWrapperCmd ${testWrapperCmd} --expected-return-code=${bt_EXPECTED_RETURN_CODE})
        endif()

        if (bt_ENV)
            foreach(env ${bt_ENV})
                set(testWrapperCmd ${testWrapperCmd} --env-var=${env})
            endforeach()
        endif()

        if (bt_PRE_PATH)
            foreach(path ${bt_PRE_PATH})
                set(testWrapperCmd ${testWrapperCmd} --pre-path=${path})
            endforeach()
        endif()

        if (bt_POST_PATH)
            foreach(path ${bt_POST_PATH})
                set(testWrapperCmd ${testWrapperCmd} --post-path=${path})
            endforeach()
        endif()
        
        # Look for resource files in the "usd" subdirectory relative to the
        # "lib" directory where the libraries are installed.
        #
        # We don't want to copy these resource files for each test, so instead
        # we set the PXR_PLUGINPATH_NAME env var to point to the "lib/usd"
        # directory where these files are installed.
        set(_plugSearchPathEnvName "PXR_PLUGINPATH_NAME")
        if (PXR_OVERRIDE_PLUGINPATH_NAME)
            set(_plugSearchPathEnvName ${PXR_OVERRIDE_PLUGINPATH_NAME})
        endif()

        set(_testPluginPath "${PXR_INSTALL_PREFIX}/maya/plugin;${CMAKE_INSTALL_PREFIX}/lib/usd")
        set(_testPrePath "$ENV{PATH};${PXR_INSTALL_PREFIX}/maya/lib;${CMAKE_INSTALL_PREFIX}/lib")

        # Ensure that Python imports the Python files built by this build.
        # On Windows convert backslash to slash and don't change semicolons
        # to colons.
        set(_testPythonPath "${CMAKE_INSTALL_PREFIX}/lib/python;${PXR_INSTALL_PREFIX}/lib/python;$ENV{PYTHONPATH}")
        if(WIN32)
            string(REGEX REPLACE "\\\\" "/" _testPythonPath "${_testPythonPath}")
            string(REGEX REPLACE "\\\\" "/" _testPluginPath "${_testPluginPath}")
            string(REGEX REPLACE "\\\\" "/" _testPrePath "${_testPrePath}")
        else()
            string(REPLACE ";" ":" _testPythonPath "${_testPythonPath}")
            string(REPLACE ";" ":" _testPluginPath "${_testPluginPath}")
            string(REPLACE ";" ":" _testPrePath "${_testPrePath}")
        endif()

        # Ensure we run with the appropriate python executable.
        if (bt_CUSTOM_PYTHON)
            set(testCmd "${bt_CUSTOM_PYTHON} ${bt_COMMAND}")
        elseif (bt_PYTHON)
            set(testCmd "${Python_EXECUTABLE} ${bt_COMMAND}")
        else()
            set(testCmd "${bt_COMMAND}")
        endif()

        add_test(
            NAME ${TEST_NAME}
            COMMAND ${Python_EXECUTABLE} ${testWrapperCmd}
                    "--env-var=PYTHONPATH=${_testPythonPath}" 
                    "--env-var=${_plugSearchPathEnvName}=${_testPluginPath}" 
                    "--pre-path=${_testPrePath}" ${testCmd}
        )

        # But in some cases, we need to pass cmake properties directly to cmake
        # run_test, rather than configuring the environment
        if (bt_RUN_SERIAL)
            set_tests_properties(${TEST_NAME} PROPERTIES RUN_SERIAL TRUE)
        endif()
    endif()
endfunction() # pxr_register_test

function(pxr_setup_plugins)
    # Install a top-level plugInfo.json in the shared area and into the 
    # top-level plugin area
    _get_resources_dir_name(resourcesDir)

    # Add extra plugInfo.json include paths to the top-level plugInfo.json,
    # relative to that top-level file.
    set(extraIncludes "")
    list(REMOVE_DUPLICATES PXR_EXTRA_PLUGINS)
    foreach(dirName ${PXR_EXTRA_PLUGINS})
        file(RELATIVE_PATH
            relDirName
            "${CMAKE_INSTALL_PREFIX}/lib/usd"
            "${CMAKE_INSTALL_PREFIX}/${dirName}"
        )
        set(extraIncludes "${extraIncludes},\n        \"${relDirName}/\"")
    endforeach()

    set(plugInfoContents "{\n    \"Includes\": [\n        \"*/${resourcesDir}/\"${extraIncludes}\n    ]\n}\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugins_plugInfo.json"
         "${plugInfoContents}")
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/plugins_plugInfo.json"
        DESTINATION ${INSTALL_DIR_SUFFIX}/lib/usd
        RENAME "plugInfo.json"
    )

    set(plugInfoContents "{\n    \"Includes\": [ \"*/${resourcesDir}/\" ]\n}\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/usd_plugInfo.json"
         "${plugInfoContents}")
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/usd_plugInfo.json"
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin/usd
        RENAME "plugInfo.json"
    )
endfunction() # pxr_setup_plugins

function(pxr_add_extra_plugins PLUGIN_AREAS)
    # Install a top-level plugInfo.json in the given plugin areas.
    _get_resources_dir_name(resourcesDir)
    set(plugInfoContents "{\n    \"Includes\": [ \"*/${resourcesDir}/\" ]\n}\n")

    get_property(help CACHE PXR_EXTRA_PLUGINS PROPERTY HELPSTRING)

    foreach(area ${PLUGIN_AREAS})
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${area}_plugInfo.json"
            "${plugInfoContents}")
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${area}_plugInfo.json"
            DESTINATION "${INSTALL_DIR_SUFFIX}/${PXR_INSTALL_SUBDIR}/${area}"
            RENAME "plugInfo.json"
        )
        list(APPEND PXR_EXTRA_PLUGINS "${PXR_INSTALL_SUBDIR}/${area}")
    endforeach()

    set(PXR_EXTRA_PLUGINS "${PXR_EXTRA_PLUGINS}" CACHE INTERNAL "${help}")
endfunction() # pxr_setup_third_plugins

function(pxr_toplevel_prologue)
    # Create a target for shared libraries.  We currently use this only
    # to test its existence.
    if(BUILD_SHARED_LIBS)
        add_custom_target(shared_libs)
    endif()

    # Create a target for targets that require Python.  Each should add
    # itself as a dependency to the "python" target.
    if(TARGET shared_libs)
        add_custom_target(python ALL)
    endif()
endfunction() # pxr_toplevel_prologue

function(pxr_toplevel_epilogue)
    # Setup the plugins in the top epilogue to ensure that everybody has had a
    # chance to update PXR_EXTRA_PLUGINS with their plugin paths.
    pxr_setup_plugins()
endfunction() # pxr_toplevel_epilogue
