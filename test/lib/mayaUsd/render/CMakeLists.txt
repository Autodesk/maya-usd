add_subdirectory(pxrUsdMayaGL)
add_subdirectory(vp2RenderDelegate)

if(BUILD_HDMAYA)
    add_subdirectory(mayaToHydra)
endif()
