# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        skelBindingsProcessor.cpp
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
        translatorSkel.cpp
        translatorUtil.cpp
        translatorXformable.cpp
        translatorXformable_decompose.cpp
)

set(HEADERS
    skelBindingsProcessor.h
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
