# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        skelBindingsProcessor.cpp
        translatorBlendShape.cpp
        translatorCamera.cpp
        translatorCurves.cpp
        translatorGprim.cpp
        translatorLight.cpp
        translatorMaterial.cpp
        translatorMayaReference.cpp
        translatorMesh.cpp
        translatorNurbsPatch.cpp
        translatorPrim.cpp
        translatorRfMLight.cpp
        translatorRfMLightDeprecated.cpp
        translatorSkel.cpp
        translatorUtil.cpp
        translatorXformable.cpp
        translatorXformableDecompose.cpp
)

set(HEADERS
    skelBindingsProcessor.h
    translatorBlendShape.h
    translatorCamera.h
    translatorCurves.h
    translatorGprim.h
    translatorLight.h
    translatorMaterial.h
    translatorMayaReference.h
    translatorMesh.h
    translatorNurbsPatch.h
    translatorPrim.h
    translatorRfMLight.h
    translatorSkel.h
    translatorUtil.h
    translatorXformable.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS 
        ${HEADERS} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/translators
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/translators
)
