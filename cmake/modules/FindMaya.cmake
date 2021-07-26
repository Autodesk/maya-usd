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
# MAYA_LIGHTAPI_VERSION Maya light API version (1 or 2)
# MAYA_HAS_DEFAULT_MATERIAL_API Presence of a default material API on MRenderItem.
# MAYA_NEW_POINT_SNAPPING_SUPPORT Presence of point new snapping support.
# MAYA_PREVIEW_RELEASE_VERSION Preview Release number (3 or more digits) in preview releases, 0 in official releases
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

    set(_MAYA_DEFINES REQUIRE_IOSTREAM _BOOL)

    if(IS_MACOSX)
        set(_MAYA_DEFINES "${_MAYA_DEFINES}" MAC_PLUGIN OSMac_ OSMac_MachO)
        set_target_properties(${target} PROPERTIES
                              PREFIX "")
    elseif(WIN32)
        set(_MAYA_DEFINES "${_MAYA_DEFINES}" _AFXDLL _MBCS NT_PLUGIN)
        set_target_properties( ${target} PROPERTIES
                               LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")
    else()
        set(_MAYA_DEFINES "${_MAYA_DEFINES}" LINUX LINUX_64)
        set_target_properties( ${target} PROPERTIES
                               PREFIX "")
    endif()
    target_compile_definitions(${target}
        PRIVATE
            ${_MAYA_DEFINES}
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

set(MAYA_LIBS_TO_FIND
    OpenMaya
    OpenMayaAnim
    OpenMayaFX
    OpenMayaRender
    OpenMayaUI
    Image
    Foundation
    IMFbase
    cg
    cgGL
    clew
)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND MAYA_LIBS_TO_FIND tbb_debug)
else()
    list(APPEND MAYA_LIBS_TO_FIND tbb)
endif()

foreach(MAYA_LIB ${MAYA_LIBS_TO_FIND})
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

if(MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MDefines.h")
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MDefines.h MAYA_PREVIEW_RELEASE_VERSION REGEX "#define MAYA_PREVIEW_RELEASE_VERSION.*$")
    if(MAYA_PREVIEW_RELEASE_VERSION)
        string(REGEX MATCHALL "[0-9]+" MAYA_PREVIEW_RELEASE_VERSION ${MAYA_PREVIEW_RELEASE_VERSION})
    else()
        set(MAYA_PREVIEW_RELEASE_VERSION 0)
    endif()
endif()

# Determine the Python version and switch between mayapy and mayapy2.
set(MAYAPY_EXE mayapy)
set(MAYA_PY_VERSION 2)
if(${MAYA_APP_VERSION} STRGREATER_EQUAL "2021")
    set(MAYA_PY_VERSION 3)

    # check to see if we have a mayapy2 executable
    find_program(MAYA_PY_EXECUTABLE2
            mayapy2
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
    if(NOT BUILD_WITH_PYTHON_3 AND MAYA_PY_EXECUTABLE2)
        set(MAYAPY_EXE mayapy2)
        set(MAYA_PY_VERSION 2)
    endif()
endif()

find_program(MAYA_PY_EXECUTABLE
        ${MAYAPY_EXE}
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

set(MAYA_LIGHTAPI_VERSION 1)
if(IS_MACOSX)
    set(MAYA_DSO_SUFFIX ".dylib")
    set(MAYA_DSO_PREFIX "lib")
elseif(IS_WINDOWS)
    set(MAYA_DSO_SUFFIX ".dll")
    set(MAYA_DSO_PREFIX "")
else(IS_LINUX)
    set(MAYA_DSO_SUFFIX ".so")
    set(MAYA_DSO_PREFIX "lib")
endif()
find_file(MAYA_OGSDEVICES_LIBRARY
        "${MAYA_DSO_PREFIX}OGSDevices${MAYA_DSO_SUFFIX}"
    HINTS
        "${MAYA_LIBRARY_DIR}"
        "${MAYA_LOCATION}"
    PATH_SUFFIXES
        lib/
        bin/
    DOC
        "Maya's ${MAYA_LIB} library path"
    # NO_CMAKE_SYSTEM_PATH needed to avoid conflicts between
    # Maya's Foundation library and OSX's framework.
    NO_CMAKE_SYSTEM_PATH
)
if (MAYA_OGSDEVICES_LIBRARY)
    file(STRINGS ${MAYA_OGSDEVICES_LIBRARY} HAS_LIGHTAPI_2 REGEX "InitializeLightShader")
    if (HAS_LIGHTAPI_2)
        set(MAYA_LIGHTAPI_VERSION 2)
    endif()
endif()
message(STATUS "Using Maya Light API Version ${MAYA_LIGHTAPI_VERSION}")

set(MAYA_HAS_DEFAULT_MATERIAL_API FALSE CACHE INTERNAL "setDefaultMaterialHandling")
if(MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MHWGeometry.h")
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MHWGeometry.h MAYA_HAS_API REGEX "setDefaultMaterialHandling")
    if(MAYA_HAS_API)
        set(MAYA_HAS_DEFAULT_MATERIAL_API TRUE CACHE INTERNAL "setDefaultMaterialHandling")
        message(STATUS "Maya has setDefaultMaterialHandling API")
    endif()
endif()

set(MAYA_NEW_POINT_SNAPPING_SUPPORT FALSE CACHE INTERNAL "snapToActive")
if (MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MSelectionContext.h")
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MSelectionContext.h MAYA_HAS_API REGEX "snapToActive")
    if(MAYA_HAS_API)
        set(MAYA_NEW_POINT_SNAPPING_SUPPORT TRUE CACHE INTERNAL "snapToActive")
        message(STATUS "Maya has new point snapping API")
    endif()
endif()

set(MAYA_HAS_CRASH_DETECTION FALSE CACHE INTERNAL "isInCrashHandler")
if(MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MGlobal.h")
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MGlobal.h MAYA_HAS_API REGEX "isInCrashHandler")
    if(MAYA_HAS_API)
        set(MAYA_HAS_CRASH_DETECTION TRUE CACHE INTERNAL "isInCrashHandler")
        message(STATUS "Maya has isInCrashHandler API")
    endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set MAYA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Maya
    REQUIRED_VARS
        MAYA_EXECUTABLE
        MAYA_PY_EXECUTABLE
        MAYA_PY_VERSION
        MAYA_INCLUDE_DIRS
        MAYA_LIBRARIES
        MAYA_API_VERSION
        MAYA_APP_VERSION
        MAYA_LIGHTAPI_VERSION
    VERSION_VAR
        MAYA_APP_VERSION
)
