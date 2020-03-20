# - Maya finder module
# This module searches for a valid Maya instalation.
# It searches for Maya's devkit, libraries, executables
# and related paths (scripts)
#
# Variables that will be defined:
# MAYA_FOUND          Defined if a Maya installation has been detected
# MAYA_EXECUTABLE     Path to Maya's executable
# MAYA_<lib>_FOUND    Defined if <lib> has been found
# MAYA_<lib>_LIBRARY  Path to <lib> library
# MAYA_INCLUDE_DIRS   Path to the devkit's include directories
# MAYA_API_VERSION    Maya version (6-8 digits)
# MAYA_APP_VERSION    Maya app version (4 digits)
#

#=============================================================================
# Copyright 2011-2012 Francisco Requena <frarees@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

#=============================================================================
# Macro for setting up typical plugin properties. These include:
#  - OS-specific plugin suffix (.mll, .so, .bundle)
#  - Removal of 'lib' prefix on osx/linux
#  - OS-specific defines
#  - Post-commnad for correcting Qt library linking on osx
#  - Windows link flags for exporting initializePlugin/uninitializePlugin
macro(maya_set_plugin_properties target)
    set_target_properties(${target} PROPERTIES
                          SUFFIX ${MAYA_PLUGIN_SUFFIX})

    set(_maya_DEFINES REQUIRE_IOSTREAM _BOOL)

    if(IS_MACOSX)
        set(_maya_DEFINES "${_maya_DEFINES}" MAC_PLUGIN OSMac_ OSMac_MachO)
        set_target_properties(${target} PROPERTIES
                              PREFIX "")
    elseif(WIN32)
        set(_maya_DEFINES "${_maya_DEFINES}" _AFXDLL _MBCS NT_PLUGIN)
        set_target_properties( ${target} PROPERTIES
                               LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")
    else()
        set(_maya_DEFINES "${_maya_DEFINES}" LINUX LINUX_64)
        set_target_properties( ${target} PROPERTIES
                               PREFIX "")
    endif()
    target_compile_definitions(${target}
        PRIVATE
            ${_maya_DEFINES}
    )
endmacro()
#=============================================================================

if(IS_MACOSX)
    set(MAYA_PLUGIN_SUFFIX ".bundle")
elseif(IS_WINDOWS)
    set(MAYA_PLUGIN_SUFFIX ".mll")
else(IS_LINUX)
    set(MAYA_PLUGIN_SUFFIX ".so")
endif()

if(IS_MACOSX)
    # On OSX, setting MAYA_LOCATION to either the base installation dir (ie,
    # `/Application/Autodesk/maya20xx`), or the Contents folder in the Maya.app dir
    # (ie, `/Application/Autodesk/maya20xx/Maya.app/Contents`) are supported.
    find_path(MAYA_BASE_DIR
            include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}/../.."
            "$ENV{MAYA_LOCATION}/../.."
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "/Applications/Autodesk/maya2020"
            "/Applications/Autodesk/maya2019"
            "/Applications/Autodesk/maya2018"
            "/Applications/Autodesk/maya2017"
            "/Applications/Autodesk/maya2016.5"
            "/Applications/Autodesk/maya2016"
        DOC
            "Maya installation root directory"
    )
    find_path(MAYA_LIBRARY_DIR
            libOpenMaya.dylib
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            MacOS/
            Maya.app/Contents/MacOS/
        DOC
            "Maya's libraries path"
    )
elseif(IS_LINUX)
    find_path(MAYA_BASE_DIR
            include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "/usr/autodesk/maya2020-x64"
            "/usr/autodesk/maya2019-x64"
            "/usr/autodesk/maya2018-x64"
            "/usr/autodesk/maya2017-x64"
            "/usr/autodesk/maya2016.5-x64"
            "/usr/autodesk/maya2016-x64"
        DOC
            "Maya installation root directory"
    )
    find_path(MAYA_LIBRARY_DIR
            libOpenMaya.so
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "Maya's libraries path"
    )
elseif(IS_WINDOWS)
    find_path(MAYA_BASE_DIR
            include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "C:/Program Files/Autodesk/Maya2020"
            "C:/Program Files/Autodesk/Maya2019"
            "C:/Program Files/Autodesk/Maya2018"
            "C:/Program Files/Autodesk/Maya2017"
            "C:/Program Files/Autodesk/Maya2016.5"
            "C:/Program Files/Autodesk/Maya2016"
        DOC
            "Maya installation root directory"
    )
    find_path(MAYA_LIBRARY_DIR
            OpenMaya.lib
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "Maya's libraries path"
    )
endif()

find_path(MAYA_INCLUDE_DIR
        maya/MFn.h
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/include/
        include/
    DOC
        "Maya's headers path"
)

find_path(MAYA_LIBRARY_DIR
        OpenMaya
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/include/
        include/
    DOC
        "Maya's libraries path"
)

list(APPEND MAYA_INCLUDE_DIRS ${MAYA_INCLUDE_DIR})

find_path(MAYA_DEVKIT_INC_DIR
       GL/glext.h
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/plug-ins/
    DOC
        "Maya's devkit headers path"
)
if(NOT "${MAYA_DEVKIT_INC_DIR}" STREQUAL "MAYA_DEVKIT_INC_DIR-NOTFOUND")
    list(APPEND MAYA_INCLUDE_DIRS ${MAYA_DEVKIT_INC_DIR})
endif()

foreach(MAYA_LIB
    OpenMaya
    OpenMayaAnim
    OpenMayaFX
    OpenMayaRender
    OpenMayaUI
    Image
    Foundation
    IMFbase
    tbb
    cg
    cgGL)

    find_library(MAYA_${MAYA_LIB}_LIBRARY
            ${MAYA_LIB}
        HINTS
            "${MAYA_LIBRARY_DIR}"
        DOC
            "Maya's ${MAYA_LIB} library path"
        # NO_CMAKE_SYSTEM_PATH needed to avoid conflicts between
        # Maya's Foundation library and OSX's framework.
        NO_CMAKE_SYSTEM_PATH
    )


    if (MAYA_${MAYA_LIB}_LIBRARY)
        list(APPEND MAYA_LIBRARIES ${MAYA_${MAYA_LIB}_LIBRARY})
    endif()
endforeach()

find_program(MAYA_EXECUTABLE
        maya
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        Maya.app/Contents/bin/
        bin/
    DOC
        "Maya's executable path"
)

find_program(MAYA_PY_EXECUTABLE
        mayapy
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        Maya.app/Contents/bin/
        bin/
    DOC
        "Maya's Python executable path"
)

if(MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MTypes.h")

    # Tease the MAYA_API_VERSION numbers from the lib headers
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MTypes.h TMP REGEX "#define MAYA_API_VERSION.*$")
    string(REGEX MATCHALL "[0-9]+" MAYA_API_VERSION ${TMP})

    # MAYA_APP_VERSION
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MTypes.h MAYA_APP_VERSION REGEX "#define MAYA_APP_VERSION.*$")
    if(MAYA_APP_VERSION)
        string(REGEX MATCHALL "[0-9]+" MAYA_APP_VERSION ${MAYA_APP_VERSION})
    else()
        string(SUBSTRING ${MAYA_API_VERSION} "0" "4" MAYA_APP_VERSION)
    endif()

endif()

# handle the QUIETLY and REQUIRED arguments and set MAYA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Maya
    REQUIRED_VARS
        MAYA_EXECUTABLE
        MAYA_PY_EXECUTABLE
        MAYA_INCLUDE_DIRS
        MAYA_LIBRARIES
    VERSION_VAR
        MAYA_API_VERSION
    VERSION_VAR
        MAYA_APP_VERSION
)