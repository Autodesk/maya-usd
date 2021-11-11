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
include(CMakeParseArguments)

# The name of the operating system for which CMake is to build
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(IS_WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(IS_LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(IS_MACOSX TRUE)
endif()

# compiler type
if (CMAKE_COMPILER_IS_GNUCXX)
    set(IS_GNU TRUE)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(IS_CLANG TRUE)
elseif(MSVC)
    set(IS_MSVC TRUE)
endif()

# Appends a path to an environment variable.
# Note: if you want to append multiple paths either call this multiple
#       times, or send in the paths with the proper platform separator.
#
#   envVar         The environment variable to modify
#   pathToAppend   The path to append
#
function(mayaUsd_append_path_to_env_var envVar pathToAppend)
    file(TO_NATIVE_PATH "${pathToAppend}" nativePathToAppend)
    if(DEFINED ENV{${envVar}})
        if(IS_WINDOWS)
            set(NEWPATH "$ENV{${envVar}};${nativePathToAppend}")
        else()
            set(NEWPATH "$ENV{${envVar}}:${nativePathToAppend}")
        endif()
        set(ENV{${envVar}} "${NEWPATH}")
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
function(mayaUsd_find_python_module module)
    string(TOUPPER ${module} module_upper)
    set(MODULE_FOUND "${module_upper}_FOUND")
    if(NOT ${MODULE_FOUND})
        if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
            set(${module}_FIND_REQUIRED TRUE)
        endif()
        execute_process(COMMAND "${Python_EXECUTABLE}" "-c"
            "from __future__ import print_function; import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
            RESULT_VARIABLE _${module}_status
            OUTPUT_VARIABLE _${module}_location
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _${module}_status)
            set(${MODULE_FOUND} ${_${module}_location} CACHE STRING
                "Location of Python module ${module}")
        endif(NOT _${module}_status)
    endif()
endfunction()

# Initialize a variable to accumulate an rpath.  The origin is the
# RUNTIME DESTINATION of the target.  If not absolute it's appended
# to CMAKE_INSTALL_PREFIX.
function(mayaUsd_init_rpath rpathRef origin)
    if(NOT IS_ABSOLUTE ${origin})
        if(DEFINED INSTALL_DIR_SUFFIX)
            set(origin "${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX}/${origin}")
        else()
            set(origin "${CMAKE_INSTALL_PREFIX}/${origin}")
        endif()
    endif()
    # mayaUsd_add_rpath uses REALPATH, so we must make sure we always
    # do so here too, to get the right relative path
    get_filename_component(origin "${origin}" REALPATH)
    set(${rpathRef} "${origin}" PARENT_SCOPE)
endfunction()

# Add a relative target path to the rpath.  If target is absolute compute
# and add a relative path from the origin to the target.
function(mayaUsd_add_rpath rpathRef target)
    if(IS_ABSOLUTE "${target}")
        # init_rpath calls get_filename_component([...] REALPATH), which does
        # symlink resolution, so we must do the same, otherwise relative path
        # determination below will fail.
        get_filename_component(target "${target}" REALPATH)
        # Make target relative to $ORIGIN (which is the first element in
        # rpath when initialized with mayaUsd_init_rpath()).
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
    set(NEW_RPATH "${${rpathRef}}")
    list(APPEND NEW_RPATH "$ORIGIN/${target}")
    set(${rpathRef} "${NEW_RPATH}" PARENT_SCOPE)
endfunction()

function(mayaUsd_install_rpath rpathRef NAME)
    # Get and remove the origin.
    list(GET ${rpathRef} 0 origin)
    set(RPATH ${${rpathRef}})
    list(REMOVE_AT RPATH 0)

    # Canonicalize and uniquify paths.
    set(FINAL "")
    foreach(path ${RPATH})
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
        if (NOT ";${FINAL};" MATCHES ";${path};")
            list(APPEND FINAL "${path}")
        endif()
    endforeach()

    set_target_properties(${NAME}
        PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "${FINAL}"
    )
endfunction()

#
# mayaUsd_promoteHeaderList(
#                        [SUBDIR  <sub-directory name>]
#                        [FILES   <list of files>]
#                        [BASEDIR <sub-directory name>])
#
#   SUBDIR     - sub-directory in which to promote files.
#   FILES      - list of files to promote.
#   BASESDIR   - base dirctory where promoted headers are installed into.
#                if not defined, mayaUsd subdirectory is used by default.
#
#
function(mayaUsd_promoteHeaderList)
    cmake_parse_arguments(PREFIX
        ""
        "SUBDIR;BASEDIR" # one_value keywords
        "HEADERS" # multi_value keywords
        ${ARGN}
    )

    if (PREFIX_HEADERS)
        set(HEADERFILES ${PREFIX_HEADERS})
    else()
        message(FATAL_ERROR "HEADERS keyword is not specified.")
    endif()

    set(BASEDIR ${CMAKE_BINARY_DIR}/include)
    if (PREFIX_BASEDIR)
        set(BASEDIR ${BASEDIR}/${PREFIX_BASEDIR})
    else()
        set(BASEDIR ${BASEDIR}/mayaUsd)
    endif()

    if (PREFIX_SUBDIR)
        set(BASEDIR ${BASEDIR}/${PREFIX_SUBDIR})
    endif()

    foreach(header ${HEADERFILES})
        set(SRCFILE ${CMAKE_CURRENT_SOURCE_DIR}/${header})
        set(DSTFILE ${BASEDIR}/${header})

        set(CONTENT "#pragma once\n#include \"${SRCFILE}\"\n")

        if (NOT EXISTS ${DSTFILE})
            message(STATUS "promoting: " ${SRCFILE})
            file(WRITE ${DSTFILE} "${CONTENT}")
        else()
            file(READ ${DSTFILE} oldContent)
            if (NOT "${CONTENT}" STREQUAL "${oldContent}")
                message(STATUS "Promoting ${SRCFILE}")
                file(WRITE ${DSTFILE} "${CONTENT}")
            endif()
        endif()
    endforeach()
endfunction()

function(mayaUsd_split_head_tail input_string split_string var_head var_tail)
    string(FIND "${input_string}" "${split_string}" head_end)
    if("${head_end}" EQUAL -1)
        message(FATAL_ERROR "input_string '${input_string}' did not contain "
            "split_string '${split_string}'")
    endif()

    string(LENGTH "${split_string}" split_string_len)
    math(EXPR tail_start "${head_end} + ${split_string_len}")

    string(SUBSTRING "${input_string}" 0 "${head_end}" "${var_head}")
    string(SUBSTRING "${input_string}" "${tail_start}" -1 "${var_tail}")
    set("${var_head}" "${${var_head}}" PARENT_SCOPE)
    set("${var_tail}" "${${var_tail}}" PARENT_SCOPE)
endfunction()

function(mayaUsd_indent outvar lines)
    string(REPLACE "\n" "\n    " lines "${lines}")
    set("${outvar}" "    ${lines}" PARENT_SCOPE)
endfunction()

# parse list arguments into a new list separated by ";" or ":"
function(separate_argument_list listName)
    if(IS_WINDOWS)
        string(REPLACE ";" "\;" ${listName} "${${listName}}")
    else(IS_LINUX OR IS_MACOSX)
        string(REPLACE ";" ":" ${listName} "${${listName}}")
    endif()
    set(${listName} "${${listName}}" PARENT_SCOPE)
endfunction()

# python extension module suffix
function(set_python_module_property target)
    if(IS_WINDOWS)
        set_target_properties(${target}
            PROPERTIES
                PREFIX ""
                SUFFIX ".pyd"
        )
    elseif(IS_LINUX OR IS_MACOSX)
        set_target_properties(${target}
            PROPERTIES
                PREFIX ""
                SUFFIX ".so"
        )
    endif()
endfunction()
