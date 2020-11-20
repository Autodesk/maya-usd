# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        debugCodes.cpp
        tokens.cpp
)

set(HEADERS
    api.h
    debugCodes.h
    id.h
    syncId.h
    tokens.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(SRCFILE ${CMAKE_CURRENT_SOURCE_DIR}/mayaUsd.h.src)
set(DSTFILE ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h)
if (NOT EXISTS ${dstFile})
    message(STATUS "promoting: " ${SRCFILE})
endif()
configure_file(${SRCFILE} ${DSTFILE})

mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR base)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/base
)

install(FILES ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)
