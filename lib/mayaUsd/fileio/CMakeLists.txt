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
        primWriter.cpp
        primWriterArgs.cpp
        primWriterContext.cpp
        primWriterRegistry.cpp
        registryHelper.cpp
        shaderReader.cpp
        shaderReaderRegistry.cpp
        shaderWriter.cpp
        shaderWriterRegistry.cpp
        transformWriter.cpp
        writeJobContext.cpp
)

# Edit as Maya requires UFE path mapping.
if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PROJECT_NAME}
        PRIVATE
            fallbackPrimUpdater.cpp
            primUpdater.cpp
            primUpdaterArgs.cpp
            primUpdaterContext.cpp
            primUpdaterRegistry.cpp
            primUpdaterManager.cpp
    )
endif()

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
    primWriter.h
    primWriterArgs.h
    primWriterContext.h
    primWriterRegistry.h
    registryHelper.h
    shaderReader.h
    shaderReaderRegistry.h
    shaderWriter.h
    shaderWriterRegistry.h
    transformWriter.h
    writeJobContext.h
)

if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    list(APPEND HEADERS
        fallbackPrimUpdater.h
        primUpdater.h
        primUpdaterArgs.h
        primUpdaterContext.h
        primUpdaterRegistry.h
        primUpdaterManager.h
    )
endif()

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

set(PYTHON_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/lib)
install(FILES inMemoryTranslation.py DESTINATION ${PYTHON_INSTALL_PREFIX})

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(chaser)
add_subdirectory(jobs)
add_subdirectory(shading)
add_subdirectory(translators)
add_subdirectory(utils)
