####################################################################################################
# Setup 
####################################################################################################

set(EVENTS_LIBRARY_NAME "AL_EventSystem")
set(EVENTS_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib) 

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

target_compile_definitions(${EVENTS_LIBRARY_NAME}
    PRIVATE
        AL_EVENT_EXPORT
)

target_include_directories(${EVENTS_LIBRARY_NAME}
    PUBLIC 
    ${EVENTS_INCLUDE_LOCATION}
)

# install the library
install(TARGETS ${EVENTS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${EVENTS_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${EVENTS_LIBRARY_LOCATION}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${EVENTS_LIBRARY_NAME}> DESTINATION ${EVENTS_LIBRARY_LOCATION} OPTIONAL)
endif()
