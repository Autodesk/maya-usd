# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
list(APPEND MAYAUSD_COMPUTESHADERS
    computeNormals.glsl
    computeNormals.cl
    plugInfo.json
)

install(FILES ${MAYAUSD_COMPUTESHADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUSD_ComputeShaders/resources
)

