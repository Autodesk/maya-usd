# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        debugCodes.cpp
)

set(headers
    api.h
    debugCodes.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(srcFile ${CMAKE_CURRENT_SOURCE_DIR}/mayaUsd.h.src)
set(dstFile ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h)
if (NOT EXISTS ${dstFile})
    message(STATUS "promoting: " ${srcFile})
endif()
configure_file(${srcFile} ${dstFile})

mayaUsd_promoteHeaderList(HEADERS ${headers} SUBDIR base)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/base
)

install(FILES ${CMAKE_BINARY_DIR}/include/mayaUsd/mayaUsd.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd
)
