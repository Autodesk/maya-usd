# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        baseExportCommand.cpp
        baseImportCommand.cpp
)

set(headers
        baseExportCommand.h
        baseImportCommand.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR commands)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/commands/
)
