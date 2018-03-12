
####################################################################################################
# Setup 
####################################################################################################

set(USDUTILS_LIBRARY_NAME "AL_USDUtils")
set(USDUTILS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# General 
####################################################################################################

list(APPEND usdutils_headers
    DebugCodes.h
    ALHalf.h
    ForwardDeclares.h
)

list(APPEND usdutils_source
    DebugCodes.cpp
)

add_library(${USDUTILS_LIBRARY_NAME}
    SHARED
        ${usdutils_source}
)

target_include_directories(${USDUTILS_LIBRARY_NAME} 
    PUBLIC
    ${UTILS_INCLUDE_LOCATION} 
    ${USDUTILS_INCLUDE_LOCATION} 
    )

target_link_libraries(${USDUTILS_LIBRARY_NAME}
  gf
  usd
  ${MAYA_Foundation_LIBRARY}
  ${MAYA_OpenMaya_LIBRARY}
)

####################################################################################################
# Install 
####################################################################################################

# install the public headers
install( FILES ${usdutils_headers}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/usd/utils
)

# install the library
install(TARGETS ${USDUTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${USDUTILS_LIBRARY_LOCATION}
)
