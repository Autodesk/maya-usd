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

function(init_usd)
    set(USD_LOCATION_OVERRIDE "" CACHE PATH "Location of USD to use")

    if (USD_LOCATION_OVERRIDE)
        set(PXR_USD_LOCATION "${USD_LOCATION_OVERRIDE}")
    endif()

    set(PXR_USD_LOCATION "${PXR_USD_LOCATION}" PARENT_SCOPE)
    set(PXR_USD_INCLUDE_DIR "${PXR_USD_LOCATION}/include" PARENT_SCOPE)
    set(PXR_USD_LIB_DIR "${PXR_USD_LOCATION}/lib" PARENT_SCOPE)
    set(USD_ROOT "${PXR_USD_LOCATION}" PARENT_SCOPE)

    # Adjust PYTHONPATH, PATH
    append_path_to_env_var("PYTHONPATH" "${PXR_USD_LOCATION}/lib/python")
    if(WIN32)
        append_path_to_env_var("PATH" "${PXR_USD_LOCATION}/bin;${PXR_USD_LOCATION}/lib")
    endif()
endfunction()

init_usd()
