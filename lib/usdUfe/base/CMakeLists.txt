# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        tokens.cpp
)

set(HEADERS
    api.h
    debugCodes.h
    tokens.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(SRCFILE ${CMAKE_CURRENT_SOURCE_DIR}/usdUfe.h.src)
set(DSTFILE ${CMAKE_BINARY_DIR}/include/usdUfe/usdUfe.h)
if (NOT EXISTS ${dstFile})
    message(STATUS "promoting: " ${SRCFILE})
endif()
configure_file(${SRCFILE} ${DSTFILE})

mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR base)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/base
)

install(FILES ${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}/usdUfe.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)
