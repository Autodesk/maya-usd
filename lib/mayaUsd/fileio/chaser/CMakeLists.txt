# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        chaser.cpp
        chaserRegistry.cpp
        importChaserRegistry.cpp
        importChaser.cpp

)

set(HEADERS
    chaser.h
    chaserRegistry.h
    importChaserRegistry.h
    importChaser.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}/fileio/chaser
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/chaser
)
