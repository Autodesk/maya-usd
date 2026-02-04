#
# Module to find AdskUsdComponentCreator.
#
# This module searches for a valid USD component creator installation.
# See the find_package at the bottom for the list of variables that will be set.
#

message(STATUS "Finding Autodesk USD Component Creator")

############################################################################
#
# C++ headers

find_path(ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR
    NAMES
        AdskUsdComponentCreator/AdskUsdComponentCreatorVersion.h
    HINTS
        $ENV{ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
        ${ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC
        "USD Component Creator C++ include headers folder"
)

############################################################################
#
# Link libraries

set(ADSK_USD_COMPONENT_CREATOR_LIBS_TO_FIND
    AdskUsdComponentCreator
    AdskVariantEditor
    sdf_tools
)

set(ADSK_USD_COMPONENT_CREATOR_LIBRARIES)

foreach(ADSK_USD_COMPONENT_CREATOR_LIB ${ADSK_USD_COMPONENT_CREATOR_LIBS_TO_FIND})
    find_library(ADSK_USD_COMPONENT_CREATOR_${ADSK_USD_COMPONENT_CREATOR_LIB}_LIBRARY
        NAMES
            ${ADSK_USD_COMPONENT_CREATOR_LIB}
        HINTS
            $ENV{ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
            ${ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
        PATH_SUFFIXES
            lib
        DOC
            "USD Component Creator ${ADSK_USD_COMPONENT_CREATOR_LIB} library"
        NO_CMAKE_SYSTEM_PATH
    )
    if (ADSK_USD_COMPONENT_CREATOR_${ADSK_USD_COMPONENT_CREATOR_LIB}_LIBRARY)
        list(APPEND ADSK_USD_COMPONENT_CREATOR_LIBRARIES ${ADSK_USD_COMPONENT_CREATOR_${ADSK_USD_COMPONENT_CREATOR_LIB}_LIBRARY})
    endif()
endforeach()

############################################################################
#
# Binaries, dynamic-libraries (DLL) and commands folder

if(IS_MACOSX)
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_SUFFIX "dylib")
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_PREFIX "lib")
elseif(IS_WINDOWS)
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_SUFFIX "dll")
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_PREFIX "")
else(IS_LINUX)
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_SUFFIX "so")
    set(ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_PREFIX "lib")
endif()

find_path(ADSK_USD_COMPONENT_CREATOR_BIN_DIR
    NAMES
        ${ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_PREFIX}AdskUsdComponentCreator.${ADSK_USD_COMPONENT_CREATOR_PLUGIN_DSO_SUFFIX}
    HINTS
        $ENV{ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
        ${ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
    PATH_SUFFIXES
        bin
    DOC
        "USD Component Creator binaries (DLL and commands) folder"
)

############################################################################
#
# Maya plugin folder

find_path(ADSK_USD_COMPONENT_CREATOR_MAYA_PLUGIN_DIR
    NAMES
        usd_component_creator.py
    HINTS
        $ENV{ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
        ${ADSK_USD_COMPONENT_CREATOR_ROOT_DIR}
    PATH_SUFFIXES
        plugin
    DOC
        "USD Component Creator Maya plugin folder"
)

############################################################################
#
# Component creator version

if(ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR)
    file(
        STRINGS
        ${ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR}/AdskUsdComponentCreator/AdskUsdComponentCreatorVersion.h
        ADSK_USD_COMPONENT_CREATOR_VERSION
        REGEX "define ADSK_USD_COMPONENT_CREATOR_VERSION .*")
    if(ADSK_USD_COMPONENT_CREATOR_VERSION)
        string(REGEX MATCHALL "[0-9.]+" ADSK_USD_COMPONENT_CREATOR_VERSION ${ADSK_USD_COMPONENT_CREATOR_VERSION})
    endif()
endif()

# Handle the QUIETLY and REQUIRED arguments and set ADSK_USD_COMPONENT_CREATOR_FOUND to TRUE if
# all listed variables are TRUE.

############################################################################
#
# Component creator package

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdComponentCreator
    REQUIRED_VARS
        ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR
        ADSK_USD_COMPONENT_CREATOR_MAYA_PLUGIN_DIR
        ADSK_USD_COMPONENT_CREATOR_BIN_DIR        
        ADSK_USD_COMPONENT_CREATOR_LIBRARIES
    VERSION_VAR
        ADSK_USD_COMPONENT_CREATOR_VERSION
)

# Report to the user where the component creator package was found.

if (AdskUsdComponentCreator_FOUND)
    message(STATUS "   Autodesk USD Component Creator version is            ${ADSK_USD_COMPONENT_CREATOR_VERSION}")
    message(STATUS "   Autodesk USD Component Creator include folder is     ${ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR}")
    message(STATUS "   Autodesk USD Component Creator Maya plugin folder is ${ADSK_USD_COMPONENT_CREATOR_MAYA_PLUGIN_DIR}")
    message(STATUS "   Autodesk USD Component Creator binaries folder is    ${ADSK_USD_COMPONENT_CREATOR_BIN_DIR}")
    message(STATUS "   Autodesk USD Component Creator libraries are         ${ADSK_USD_COMPONENT_CREATOR_LIBRARIES}")
endif()
