set(TARGET_NAME "mayaUsdUtils")

set(MAYAUSDUTILS_PYTHON_LIBRARY_NAME _${TARGET_NAME})
set(MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/python/${TARGET_NAME})

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        DebugCodes.cpp
        DiffCore.cpp
        MayaTransformAPI.cpp
        util.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYA_USD_UTILS_EXPORT
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${CMAKE_BINARY_DIR}/include
    INTERFACE
        ${USD_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PUBLIC
        gf
        usd
        sdf
        usdGeom
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
set(headers
    ALHalf.h
    Api.h
    DebugCodes.h
    DiffCore.h
    ForwardDeclares.h
    MayaTransformAPI.h
    SIMD.h
    util.h
)

mayaUsd_promoteHeaderList( 
    HEADERS
        ${headers}
    BASEDIR
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES ${headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------

add_library(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME} SHARED)

target_sources(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
PRIVATE
  module.cpp
  wrapMayaTransformAPI.cpp
)

# -----------------------------------------------------------------------------
# python init
# -----------------------------------------------------------------------------

set(PY_INIT_FILES
  __init__.py
)

mayaUsd_copyFiles(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  DESTINATION 
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}
  FILES
    ${PY_INIT_FILES}
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  PRIVATE
    MFB_PACKAGE_NAME=${TARGET_NAME}
    MFB_ALT_PACKAGE_NAME=${TARGET_NAME}
    MFB_PACKAGE_MODULE=${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME} 
  PUBLIC
    ${UTILS_INCLUDE_LOCATION} 
    ${MAYAUSDUTILS_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
    )

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  PRIVATE
    ${TARGET_NAME}
    ${python_LIBRARIES}
    usdGeom
)

# -----------------------------------------------------------------------------
# target properties
# -----------------------------------------------------------------------------
set_target_properties(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  PROPERTIES 
    OUTPUT_NAME ${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}
)

set_python_module_property(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME})

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
foreach(INPUT_FILE ${PY_INIT_FILES})
  string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}/lib/python/${TARGET_NAME} OUTPUT_FILE ${INPUT_FILE})
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE})")
  install(CODE "execute_process(COMMAND python -m compileall ${OUTPUT_FILE} ${CMAKE_INSTALL_PREFIX})")
endforeach()

install(TARGETS ${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION}
)