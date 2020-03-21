# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        fallbackPrimReader.cpp
        functorPrimReader.cpp
        functorPrimWriter.cpp
        importData.cpp
        instancedNodeWriter.cpp
        primReader.cpp
        primReaderArgs.cpp
        primReaderContext.cpp
        primReaderRegistry.cpp
        primUpdater.cpp
        primUpdaterContext.cpp
        primUpdaterRegistry.cpp
        primWriter.cpp
        primWriterArgs.cpp
        primWriterContext.cpp
        primWriterRegistry.cpp
        registryHelper.cpp
        shaderWriter.cpp
        transformWriter.cpp
        writeJobContext.cpp
        chaser/chaser.cpp
        chaser/chaserRegistry.cpp
        jobs/jobArgs.cpp
        jobs/modelKindProcessor.cpp
        jobs/readJob.cpp
        jobs/writeJob.cpp
        shading/shadingModeDisplayColor.cpp
        shading/shadingModeExporter.cpp
        shading/shadingModeExporterContext.cpp
        shading/shadingModeImporter.cpp
        shading/shadingModePxrRis.cpp
        shading/shadingModeRegistry.cpp
        shading/shadingModeUseRegistry.cpp
        translators/skelBindingsProcessor.cpp
        translators/translatorCamera.cpp
        translators/translatorCurves.cpp
        translators/translatorGprim.cpp
        translators/translatorMaterial.cpp
        translators/translatorMayaReference.cpp
        translators/translatorMesh.cpp
        translators/translatorNurbsPatch.cpp
        translators/translatorPrim.cpp
        translators/translatorRfMLight.cpp
        translators/translatorSkel.cpp
        translators/translatorUtil.cpp
        translators/translatorXformable.cpp
        translators/translatorXformable_decompose.cpp
        utils/adaptor.cpp
        utils/meshUtil.cpp
        utils/readUtil.cpp
        utils/roundTripUtil.cpp
        utils/shadingUtil.cpp
        utils/userTaggedAttribute.cpp
        utils/writeUtil.cpp
        utils/xformStack.cpp
)

set(headers
    fallbackPrimReader.h
    functorPrimReader.h
    functorPrimWriter.h
    importData.h
    instancedNodeWriter.h
    primReader.h
    primReaderArgs.h
    primReaderContext.h
    primReaderRegistry.h
    primUpdater.h
    primUpdaterContext.h
    primUpdaterRegistry.h
    primWriter.h
    primWriterArgs.h
    primWriterContext.h
    primWriterRegistry.h
    registryHelper.h
    shaderWriter.h
    transformWriter.h
    writeJobContext.h
    chaser/chaser.h
    chaser/chaserRegistry.h
    jobs/jobArgs.h
    jobs/modelKindProcessor.h
    jobs/readJob.h
    jobs/writeJob.h
    shading/shadingModeExporterContext.h
    shading/shadingModeExporter.h
    shading/shadingModeImporter.h
    shading/shadingModeRegistry.h
    utils/adaptor.h
    utils/meshUtil.h
    utils/readUtil.h
    utils/roundTripUtil.h
    utils/shadingUtil.h
    utils/userTaggedAttribute.h
    utils/writeUtil.h
    utils/xformStack.h
    translators/skelBindingsProcessor.h
    translators/translatorCamera.h
    translators/translatorCurves.h
    translators/translatorGprim.h
    translators/translatorMaterial.h
    translators/translatorMayaReference.h
    translators/translatorMesh.h
    translators/translatorNurbsPatch.h
    translators/translatorPrim.h
    translators/translatorRfMLight.h
    translators/translatorSkel.h
    translators/translatorUtil.h
    translators/translatorXformable.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR fileio)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/fileio/
)
