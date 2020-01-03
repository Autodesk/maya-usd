# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
       notice.cpp
       proxyShapeNotice.cpp
       stageNoticeListener.cpp
)

set(headers
    notice.h
    proxyShapeNotice.h
    stageNoticeListener.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(${TARGET_NAME} HEADERS ${headers} DIRNAME listeners)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${headers}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/listeners/
)