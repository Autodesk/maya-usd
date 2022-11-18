if(BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(pxrUsdMayaGL)
    add_subdirectory(vp2RenderDelegate)
endif()

if(BUILD_HDMAYA)
    add_subdirectory(mayaToHydra)
endif()
