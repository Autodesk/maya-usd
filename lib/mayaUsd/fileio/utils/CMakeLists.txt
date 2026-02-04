# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        adaptor.cpp
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

if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
    target_sources(${PROJECT_NAME}
        PRIVATE
            orphanedNodesManagerUtil.cpp
            proxyAccessorUtil.cpp
    )
endif()

set(HEADERS
    adaptor.h
    jointWriteUtils.h
    meshReadUtils.h
    meshWriteUtils.h
    readUtil.h
    roundTripUtil.h
    shadingUtil.h
    splineUtils.h
    userTaggedAttribute.h
    writeUtil.h
    xformStack.h
)

if(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR)
    list(APPEND HEADERS
        orphanedNodesManagerUtil.h
        proxyAccessorUtil.h
    )
endif()

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
