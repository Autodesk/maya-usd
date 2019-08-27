#
# =======================================================================
# Copyright 2019 Autodesk, Inc. All rights reserved.
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
# =======================================================================
#

# The name of the operating system for which CMake is to build
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(IS_WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(IS_LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(IS_MACOSX TRUE)
endif()

# Appends a path to an environment variable.
# Note: if you want to append multiple paths either call this multiple
#       times, or send in the paths with the proper platform separator.
#
#   envVar         The environment variable to modify
#   pathToAppend   The path to append
#
function(append_path_to_env_var envVar pathToAppend)
    file(TO_NATIVE_PATH "${pathToAppend}" nativePathToAppend)
    if(DEFINED ENV{${envVar}})
        if(IS_WINDOWS)
            set(newPath "$ENV{${envVar}};${nativePathToAppend}")
        else()
            set(newPath "$ENV{${envVar}}:${nativePathToAppend}")
        endif()
        set(ENV{${envVar}} "${newPath}")
    else()
        set(ENV{${envVar}} "${nativePathToAppend}")
    endif()
    message("Updated ${envVar}: $ENV{${envVar}}")
endfunction()

# Finds if a specific Python module is installed in the current Python.
# <module>_FOUND will be set to indicate whether the module was found.
#
# module           The python module to find
#
function(find_python_module module)
    string(TOUPPER ${module} module_upper)
    set(module_found "${module_upper}_FOUND")
    if(NOT ${module_found})
        if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
            set(${module}_FIND_REQUIRED TRUE)
        endif()
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
            "import re, ${module}; print re.compile('/__init__.py.*').sub('',${module}.__file__)"
            RESULT_VARIABLE _${module}_status
            OUTPUT_VARIABLE _${module}_location
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _${module}_status)
            set(${module_found} ${_${module}_location} CACHE STRING
                "Location of Python module ${module}")
        endif(NOT _${module}_status)
    endif(NOT ${module_found})
endfunction(find_python_module)

# Initialize a variable to accumulate an rpath.  The origin is the
# RUNTIME DESTINATION of the target.  If not absolute it's appended
# to CMAKE_INSTALL_PREFIX.
function(init_rpath rpathRef origin)
    if(NOT IS_ABSOLUTE ${origin})
        set(origin "${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX}/${origin}")
        get_filename_component(origin "${origin}" REALPATH)
    endif()
    set(${rpathRef} "${origin}" PARENT_SCOPE)
endfunction()

# Add a relative target path to the rpath.  If target is absolute compute
# and add a relative path from the origin to the target.
function(add_rpath rpathRef target)
    if(IS_ABSOLUTE "${target}")
        # Make target relative to $ORIGIN (which is the first element in
        # rpath when initialized with _pxr_init_rpath()).
        list(GET ${rpathRef} 0 origin)
        file(RELATIVE_PATH
            target
            "${origin}"
            "${target}"
        )
        if("x${target}" STREQUAL "x")
            set(target ".")
        endif()
    endif()
    file(TO_CMAKE_PATH "${target}" target)
    set(new_rpath "${${rpathRef}}")
    list(APPEND new_rpath "$ORIGIN/${target}")
    set(${rpathRef} "${new_rpath}" PARENT_SCOPE)
endfunction()

function(install_rpath rpathRef NAME)
    # Get and remove the origin.
    list(GET ${rpathRef} 0 origin)
    set(rpath ${${rpathRef}})
    list(REMOVE_AT rpath 0)

    # Canonicalize and uniquify paths.
    set(final "")
    foreach(path ${rpath})
        # Replace $ORIGIN with @loader_path
        if(IS_MACOSX)
            if("${path}/" MATCHES "^[$]ORIGIN/")
                # Replace with origin path.
                string(REPLACE "$ORIGIN/" "@loader_path/" path "${path}/")
            endif()
        endif()

        # Strip trailing slashes.
        string(REGEX REPLACE "/+$" "" path "${path}")

        # Ignore paths we already have.
        if (NOT ";${final};" MATCHES ";${path};")
            list(APPEND final "${path}")
        endif()
    endforeach()

    set_target_properties(${NAME}
        PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${final}"
    )
endfunction()

function(promoteMayaUsdHeader)
    set(srcFile ${CMAKE_CURRENT_SOURCE_DIR}/base/mayaUsd.h.src)
    set(dstFile ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h)
    if (NOT EXISTS ${dstFile})
        message(STATUS "promoting: " ${srcFile})
    endif()
    configure_file(${srcFile} ${dstFile})
endfunction()

function(promoteHeaderList)
    foreach(header ${ARGV})
        set(srcFile ${CMAKE_CURRENT_SOURCE_DIR}/${header})
        set(dstFile ${CMAKE_BINARY_DIR}/include/mayaUsd/${header})
        if (NOT EXISTS ${dstFile})
            message(STATUS "promoting: " ${srcFile})
        endif()
        configure_file(${srcFile} ${dstFile} COPYONLY)
    endforeach()
endfunction()
