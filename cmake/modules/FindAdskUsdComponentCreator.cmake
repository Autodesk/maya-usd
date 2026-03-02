#
# Module to find AdskUsdComponentCreator.
#
# This module searches for a valid USD Component Creator installation.
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
        "Component Creator header path"
)

############################################################################
#
# Link libraries

set(ADSK_USD_COMPONENT_CREATOR_LIBS_TO_FIND
    AdskUsdComponentCreator
    AdskVariantEditor
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

###########################################################################
#
# Component Creator version

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

############################################################################
#
# Component creator package
#
# Handle the QUIETLY and REQUIRED arguments and set AdskUsdComponentCreator_FOUND
# to TRUE if all listed variables are TRUE.

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AdskUsdComponentCreator
    REQUIRED_VARS
        ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR
        ADSK_USD_COMPONENT_CREATOR_LIBRARIES
    VERSION_VAR
        ADSK_USD_COMPONENT_CREATOR_VERSION
)

# Report to the user where the package was found.

if (AdskUsdComponentCreator_FOUND)
    # This will follow a message "-- Found AdskUsdComponentCreator: <path> ..."
    message(STATUS "  Version: ${ADSK_USD_COMPONENT_CREATOR_VERSION}")
    message(STATUS "  Include dir: ${ADSK_USD_COMPONENT_CREATOR_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${ADSK_USD_COMPONENT_CREATOR_LIBRARIES}")
endif()
