#-
#*****************************************************************************
# Copyright 2018 Autodesk, Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc. and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
#*****************************************************************************
#+

#------------------------------------------------------------------------------
#
# Creates an IMPORTED library named UFE.
#
# Linking against UFE will cause the proper include directories
# to be added to your target.
#
function(init_ufe)

    include(cmake/ufe_version.info)

    set(UFE_VERSION "${UFE_MAJOR_VERSION}.${UFE_MINOR_VERSION}.${UFE_PATCH_LEVEL}"
        CACHE STRING "The version of UFE used by MayaUFEPlugin")

    set (UFE_SMALL_VERSION "${UFE_MAJOR_VERSION}_${UFE_MINOR_VERSION}")

    #--------------------------------------------------------------------------
    # Allow a developer to override the UFE Location to test using a local
    # build of UFE instead of one that has been download from Artifactory
    set(UFE_LOCATION_OVERRIDE "" CACHE PATH
        "Location of UFE to use (in replacement of the Artifactory one.")

    if (UFE_LOCATION_OVERRIDE)
        set(UFE_LOCATION "${UFE_LOCATION_OVERRIDE}")
    else()
        peptide_artifactory_download_zip(
            "ufe-${UFE_VERSION}-${PEPTIDE_SHORTVARIANT_DIR}-${PEPTIDE_SHORT_OSNAME}"
            UFE_LOCATION
            TOP_LEVEL_DIR "ufe-${UFE_VERSION}")
    endif()

    set(UFE_LOCATION "${UFE_LOCATION}" PARENT_SCOPE)

    set(UFE_INCLUDE_DIR  "${UFE_LOCATION}/common/include" PARENT_SCOPE)

    if(PEPTIDE_IS_DEBUG)
        set(UFE_VARIANT_NAME ${PEPTIDE_VARIANT_DIR})
    else()
        set(UFE_VARIANT_NAME RelWithDebInfo)
    endif()

    if(PEPTIDE_IS_WINDOWS)
        set(UFE_LIBRARY_DIR      "${UFE_LOCATION}/platform/${CMAKE_SYSTEM_NAME}/${UFE_VARIANT_NAME}/lib")
    else()
        set(UFE_LIBRARY_DIR      "${UFE_LOCATION}/platform/${CMAKE_SYSTEM_NAME}/${UFE_VARIANT_NAME}/lib")
    endif()
    set(UFE_LIBRARY_DIR "${UFE_LIBRARY_DIR}" PARENT_SCOPE)

    if(NOT PEPTIDE_IS_WINDOWS)
        set(MAYA_UFE_PLUGIN_UFE_LIB_PREFIX "lib")
    else()
        set(MAYA_UFE_PLUGIN_UFE_LIB_PREFIX "")
    endif()

    add_library(UFE SHARED IMPORTED)
    set(UFE_LIB_NAME ${MAYA_UFE_PLUGIN_UFE_LIB_PREFIX}UFE_${UFE_SMALL_VERSION}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(IMPORTLIB "${UFE_LIBRARY_DIR}/${UFE_LIB_NAME}")
    set_property(TARGET UFE PROPERTY IMPORTED_LOCATION "${IMPORTLIB}")
    set_property(TARGET UFE PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${UFE_INCLUDE_DIR}")
    if(PEPTIDE_IS_WINDOWS)
        set_property(TARGET UFE PROPERTY IMPORTED_IMPLIB "${UFE_LOCATION}/platform/${CMAKE_SYSTEM_NAME}/${UFE_VARIANT_NAME}/lib/UFE_${UFE_SMALL_VERSION}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    if(PEPTIDE_IS_WINDOWS)
        set(SHAREDLIB "${PEPTIDE_OUTPUT_BIN_DIR}/${UFE_LIB_NAME}")
    else()
        set(SHAREDLIB "${PEPTIDE_OUTPUT_LIB_DIR}/${UFE_LIB_NAME}")
    endif()
	
endfunction()

init_ufe()
