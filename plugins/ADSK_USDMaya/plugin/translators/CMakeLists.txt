set(PXR_PACKAGE mayaUsdTranslators)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        arch
        gf
        kind
        sdf
        tf
        usd
        usdGeom
        usdLux
        usdRi
        usdShade
        usdSkel
        usdUtils
        vt
        ${Boost_PYTHON_LIBRARY}
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}

    PRIVATE_CLASSES
        cameraWriter
        fileTextureWriter
        instancerWriter
        jointWriter
        locatorWriter
        meshWriter
        nurbsCurveWriter
        nurbsSurfaceWriter
        particleWriter

    CPPFILES
        cameraReader.cpp
        lightReader.cpp
        lightWriter.cpp
        materialReader.cpp
        meshReader.cpp
        meshWriter_Primvars.cpp
        meshWriter_Skin.cpp
        meshWriter_Subdiv.cpp
        nurbsCurvesReader.cpp
        nurbsPatchReader.cpp
        plugin.cpp
        scopeReader.cpp
        skelReader.cpp
        xformReader.cpp
        mayaReferenceReader.cpp

    PRIVATE_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

