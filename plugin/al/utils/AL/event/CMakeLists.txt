####################################################################################################
# Event Source
####################################################################################################
list(APPEND utils_event_headers
    Api.h
    EventHandler.h
)

list(APPEND utils_event_source
    ${CMAKE_CURRENT_SOURCE_DIR}/EventHandler.cpp
)

set(utils_source ${utils_source} ${utils_event_source} PARENT_SCOPE)


install( FILES ${utils_event_headers}
          DESTINATION ${AL_INSTALL_PREFIX}/include/AL/event )