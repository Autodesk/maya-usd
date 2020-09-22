set(PXR_PACKAGE pxrUsdPreviewSurface)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        arch
        basePxrUsdPreviewSurface
        mayaUsd
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

set(PREVIEWSURFACE_MAYA_TYPE_NAME "pxrUsdPreviewSurface")

configure_file("${PROJECT_SOURCE_DIR}/lib/usd/pxrUsdPreviewSurface/AEusdPreviewSurfaceTemplate.mel"
               "${CMAKE_CURRENT_BINARY_DIR}/AEpxrUsdPreviewSurfaceTemplate.mel"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AEpxrUsdPreviewSurfaceTemplate.mel
    DESTINATION "${PXR_INSTALL_PREFIX}/${PXR_INSTALL_SUBDIR}/lib/usd/usdMaya/resources"
)

