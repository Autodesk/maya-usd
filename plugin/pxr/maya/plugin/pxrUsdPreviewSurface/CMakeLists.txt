set(PXR_PACKAGE pxrUsdPreviewSurface)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        arch
        basePxrUsdPreviewSurface
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    CPPFILES
        plugin.cpp

    PRIVATE_HEADERS
        api.h

    DISABLE_PRECOMPILED_HEADERS
)
