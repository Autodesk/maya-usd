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
# Creates an IMPORTED library named USD.
#
# Linking against USD will cause the proper include directories
# to be added to your target.
#
function(init_usd)

    include(cmake/usd_version.info)

    set(USD_VERSION "${USD_MAJOR_VERSION}.${USD_MINOR_VERSION}.${USD_PATCH_LEVEL}"
        CACHE STRING "The version of USD used by AL_USDMaya")

    set (USD_SMALL_VERSION "${USD_MAJOR_VERSION}_${USD_MINOR_VERSION}")

    #--------------------------------------------------------------------------
    # Allow a developer to override the USD Location to test using a local
    # build of USD instead of one that has been download from Artifactory
    set(USD_LOCATION_OVERRIDE "" CACHE PATH
        "Location of USD to use (in replacement of the Artifactory one.")

    if (USD_LOCATION_OVERRIDE)
        set(AL_USDMAYA_USD_LOCATION "${USD_LOCATION_OVERRIDE}")
    else()
        peptide_artifactory_download_zip(
            "usd-${USD_VERSION}-${PEPTIDE_SHORTVARIANT_DIR}-${PEPTIDE_SHORT_OSNAME}"
            AL_USDMAYA_USD_LOCATION 
            TARGET_DIR "usd" )
    endif()

    set(AL_USDMAYA_USD_LOCATION "${AL_USDMAYA_USD_LOCATION}" PARENT_SCOPE)
    set(USD_ROOT "${AL_USDMAYA_USD_LOCATION}" PARENT_SCOPE)

    set(AL_USDMAYA_USD_INCLUDE_DIR  "${AL_USDMAYA_USD_LOCATION}/include")

    if(PEPTIDE_IS_WINDOWS)
        set(AL_USDMAYA_USD_LIB_DIR      "${AL_USDMAYA_USD_LOCATION}/bin")
    else()
        set(AL_USDMAYA_USD_LIB_DIR      "${AL_USDMAYA_USD_LOCATION}/lib")
    endif()
    set(AL_USDMAYA_USD_LIB_DIR "${AL_USDMAYA_USD_LIB_DIR}" PARENT_SCOPE)

	
endfunction()

init_usd()
