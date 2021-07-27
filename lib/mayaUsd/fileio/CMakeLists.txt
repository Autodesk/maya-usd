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
        schemaApiWriter.cpp
        schemaApiWriterRegistry.cpp
        shaderReader.cpp
        shaderReaderRegistry.cpp
        shaderWriter.cpp
        shaderWriterRegistry.cpp
        transformWriter.cpp
        writeJobContext.cpp
)

set(HEADERS
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
    schemaApiWriter.h
    schemaApiWriterRegistry.h
    shaderReader.h
    shaderReaderRegistry.h
    shaderWriter.h
    shaderWriterRegistry.h
    transformWriter.h
    writeJobContext.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR fileio)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio
)

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(chaser)
add_subdirectory(jobs)
add_subdirectory(shading)
add_subdirectory(translators)
add_subdirectory(utils)
