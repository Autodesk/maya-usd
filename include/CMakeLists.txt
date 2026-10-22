# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Special file for handling boost vs pxr::boost
#------------------------------------------------------------------------------
set(TOP_HEADERS
    cxx17_legacy_support.h
    pxr_python.h
)

mayaUsd_promoteHeaderList(HEADERS ${TOP_HEADERS} BASEDIR "NONE")

install(FILES ${TOP_HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

if (BUILD_MAYAUSD_LIBRARY OR BUILD_AL_PLUGIN OR BUILD_USDLAYEREDITOR)
    #------------------------------------------------------------------------------
    # Gulrak filesystem
    #------------------------------------------------------------------------------
    set(GULRAK_HEADERS
        fs_std.hpp  
        filesystem.hpp
    )

    mayaUsd_promoteHeaderList(HEADERS ${GULRAK_HEADERS} BASEDIR "ghc")

    # The standalone usdLayerEditor only needs them to build, not in its install.
    if (BUILD_MAYAUSD_LIBRARY OR BUILD_AL_PLUGIN)
        install(FILES ${GULRAK_HEADERS}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ghc
        )
    endif()
endif()
