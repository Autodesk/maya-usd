####################################################################################################
# Setup 
####################################################################################################

set(EVENTS_LIBRARY_NAME "AL_EventSystem")
set(EVENTS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

# The list that will be populated with sources, sources will be the ones compiled into the library
list(APPEND utils_source)

####################################################################################################
# Append child sources
####################################################################################################

# Append event sources
add_subdirectory(event)

####################################################################################################
# Utils Library
####################################################################################################

add_library(${EVENTS_LIBRARY_NAME}
    SHARED
        ${utils_source}
)

target_include_directories(${EVENTS_LIBRARY_NAME}
    PUBLIC 
    ${EVENTS_INCLUDE_LOCATION}
)

# install the library
install(TARGETS ${EVENTS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${EVENTS_LIBRARY_LOCATION}
)