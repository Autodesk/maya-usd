
####################################################################################################
# Setup 
####################################################################################################

set(USDUTILS_LIBRARY_NAME "AL_USDUtils")
set(USDUTILS_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib) 

set(USDUTILS_PYTHON_LIBRARY_NAME ${USDUTILS_LIBRARY_NAME}Python)
set(USDUTILS_PYTHON_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/python/AL/usd/utils)

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
    MayaTransformAPI.h
)

list(APPEND usdutils_source
    DebugCodes.cpp
    DiffCore.cpp
    module.cpp
    MayaTransformAPI.cpp
    wrapMayaTransformAPI.cpp
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
)

list(APPEND usdutils_python_source
  module.cpp
  wrapMayaTransformAPI.cpp
)

add_library(${USDUTILS_PYTHON_LIBRARY_NAME}
  SHARED
    ${usdutils_python_source}
)

target_compile_definitions(${USDUTILS_PYTHON_LIBRARY_NAME}
  PRIVATE
    MFB_PACKAGE_NAME=AL_USDUtils
    MFB_ALT_PACKAGE_NAME=AL_USDUtils
    MFB_PACKAGE_MODULE=AL_USDUtils
)

target_include_directories(${USDUTILS_PYTHON_LIBRARY_NAME} 
    PUBLIC
    ${UTILS_INCLUDE_LOCATION} 
    ${USDUTILS_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
    )
 
target_link_libraries(${USDUTILS_PYTHON_LIBRARY_NAME}
  ${USDUTILS_LIBRARY_NAME}
  ${python_LIBRARIES}
  tf
  gf
  usd
  usdGeom
)

set_target_properties(${USDUTILS_PYTHON_LIBRARY_NAME}
  PROPERTIES
    PREFIX "_"
    SUFFIX ".so"
    OUTPUT_NAME "AL_USDUtils"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

#if(NOT SKIP_USDMAYA_TESTS)

add_subdirectory(tests)
#endif()


####################################################################################################
# Install 
####################################################################################################

# install the public headers
install( FILES ${usdutils_headers}
    DESTINATION ${AL_INSTALL_PREFIX}/include/AL/usd/utils
)

# install the library
install(TARGETS ${USDUTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${USDUTILS_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${USDUTILS_LIBRARY_LOCATION}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${USDUTILS_LIBRARY_NAME}> DESTINATION ${USDUTILS_LIBRARY_LOCATION} OPTIONAL)
endif()

# install python library
install(TARGETS ${USDUTILS_PYTHON_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${USDUTILS_PYTHON_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${USDUTILS_PYTHON_LIBRARY_LOCATION}
)
