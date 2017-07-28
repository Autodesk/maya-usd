set(PXR_PACKAGE AL_USDMayaPlugin)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        usdImaging
        usdImagingGL
        AL_USDMaya
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}
        "../../lib/AL_USDMaya"

    CPPFILES
        plugin.cpp
)

