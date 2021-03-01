# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PUBLIC
        md5.c
    PRIVATE
        adaptor.cpp
        hashUtil.cpp
        jointWriteUtils.cpp
        meshReadUtils.cpp
        meshWriteUtils.cpp
        readUtil.cpp
        roundTripUtil.cpp
        shadingUtil.cpp
        userTaggedAttribute.cpp
        writeUtil.cpp
        xformStack.cpp
)

set(HEADERS
    md5.h
    adaptor.h
    hashUtil.h
    jointWriteUtils.h
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
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}/fileio/utils
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${HEADERS}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/fileio/utils
)
