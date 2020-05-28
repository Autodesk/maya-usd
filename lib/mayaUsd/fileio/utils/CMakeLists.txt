# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        adaptor.cpp
        meshReadUtils.cpp
        meshWriteUtils.cpp
        readUtil.cpp
        roundTripUtil.cpp
        shadingUtil.cpp
        userTaggedAttribute.cpp
        writeUtil.cpp
        xformStack.cpp
)

set(headers
    adaptor.h
    meshReadUtils.h
    meshWriteUtils.h
    readUtil.h
    roundTripUtil.h
    shadingUtil.h
    userTaggedAttribute.h
    writeUtil.h
    xformStack.h
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(
    HEADERS 
        ${headers} 
    BASEDIR 
        ${PROJECT_NAME}/fileio/utils
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/utils
)
