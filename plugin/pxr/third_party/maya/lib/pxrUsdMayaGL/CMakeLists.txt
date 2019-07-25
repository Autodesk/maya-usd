set(PXR_PACKAGE pxrUsdMayaGL)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        arch
        gf
        glf
        hd
        hdSt
        hdx
        px_vp20
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdImagingGL
        usdMaya
        vt
        ${Boost_LIBRARIES}
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        instancerImager
        instancerShapeAdapter

    DISABLE_PRECOMPILED_HEADERS
)

