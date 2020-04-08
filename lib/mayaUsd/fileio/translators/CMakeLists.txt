# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        skelBindingsProcessor.cpp
        translatorCamera.cpp
        translatorCurves.cpp
        translatorGprim.cpp
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

set(headers
    skelBindingsProcessor.h
    translatorCamera.h
    translatorCurves.h
    translatorGprim.h
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
        ${headers} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/translators
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/translators
)
