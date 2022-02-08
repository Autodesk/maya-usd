# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        jobArgs.cpp
        meshDataReadJob.cpp
        modelKindProcessor.cpp
        readJob.cpp
        writeJob.cpp
)

set(HEADERS
    jobArgs.h
    meshDataReadJob.h
    modelKindProcessor.h
    readJob.h
    writeJob.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS 
        ${HEADERS} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/jobs
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/jobs
)
