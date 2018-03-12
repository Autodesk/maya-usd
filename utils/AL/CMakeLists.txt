####################################################################################################
# Setup 
####################################################################################################

set(UTILS_LIBRARY_NAME "AL_Utils")
set(UTILS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

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

add_library(${UTILS_LIBRARY_NAME}
    SHARED
        ${utils_source}
)

target_include_directories(${UTILS_LIBRARY_NAME}
    PUBLIC 
    ${UTILS_INCLUDE_LOCATION}
)

# install the library
install(TARGETS ${UTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${UTILS_LIBRARY_LOCATION}
)