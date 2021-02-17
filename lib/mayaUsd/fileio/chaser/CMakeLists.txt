# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        exportChaser.cpp
        exportChaserRegistry.cpp
        importChaserRegistry.cpp
        importChaser.cpp

)

set(HEADERS
    exportChaser.h
    exportChaserRegistry.h
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
