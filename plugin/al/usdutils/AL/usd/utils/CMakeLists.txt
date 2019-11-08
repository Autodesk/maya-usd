
####################################################################################################
# Setup 
####################################################################################################

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX} )

set(USDUTILS_LIBRARY_NAME "AL_USDUtils")
set(USDUTILS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# General 
####################################################################################################

list(APPEND usdutils_headers
    Api.h
    DebugCodes.h
    ALHalf.h
    DiffCore.h
    ForwardDeclares.h
    SIMD.h
)

list(APPEND usdutils_source
    DebugCodes.cpp
    DiffCore.cpp
)

add_library(${USDUTILS_LIBRARY_NAME}
    SHARED
        ${usdutils_source}
)

target_compile_definitions(${USDUTILS_LIBRARY_NAME}
    PRIVATE
        AL_USD_UTILS_EXPORT
)

target_include_directories(${USDUTILS_LIBRARY_NAME} 
    PUBLIC
    ${UTILS_INCLUDE_LOCATION} 
    ${USDUTILS_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
    )

target_link_libraries(${USDUTILS_LIBRARY_NAME}
  gf
  usd
  ${PYTHON_LIBRARIES}
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
    RUNTIME
    DESTINATION ${USDUTILS_LIBRARY_LOCATION}
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${USDUTILS_LIBRARY_NAME}> DESTINATION ${USDUTILS_LIBRARY_LOCATION} OPTIONAL)
endif()
