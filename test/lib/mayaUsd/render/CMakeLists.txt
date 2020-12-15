add_subdirectory(pxrUsdMayaGL)

if(BUILD_HDMAYA)
    # skip tests for usd<=19.11, as these do image diffs, and it seems like
    # something having to do with colorspaces (or perhaps default lighting?)
    # changed in 20.02
    if(USD_VERSION_NUM GREATER 1911)
        add_subdirectory(mayaToHydra)
    endif()
endif()
