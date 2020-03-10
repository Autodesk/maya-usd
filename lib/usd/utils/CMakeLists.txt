set(TARGET_NAME "mayaUsdUtils")

add_library(${TARGET_NAME} SHARED)

set(MAYAUSDUTILS_PYTHON_LIBRARY_NAME _${TARGET_NAME})
set(MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/python/${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        MayaTransformAPI.cpp
        DebugCodes.cpp
        DiffCore.cpp
        wrapMayaTransformAPI.cpp
        module.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYA_USD_UTILS_EXPORT
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
# python
# -----------------------------------------------------------------------------

set(PY_INIT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
)

# copy init files to build location for module tests
foreach(INPUT_FILE ${PY_INIT_FILES})
  string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME} OUTPUT_FILE ${INPUT_FILE})
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE})
endforeach()


list(APPEND mayausdutils_python_source
  module.cpp
  wrapMayaTransformAPI.cpp
)

add_library(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  SHARED
    ${mayausdutils_python_source}
)

target_compile_definitions(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  PRIVATE
    MFB_PACKAGE_NAME=${TARGET_NAME}
    MFB_ALT_PACKAGE_NAME=${TARGET_NAME}
    MFB_PACKAGE_MODULE=${TARGET_NAME}
)

target_include_directories(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME} 
    PUBLIC
    ${UTILS_INCLUDE_LOCATION} 
    ${MAYAUSDUTILS_INCLUDE_LOCATION} 
    ${USD_INCLUDE_DIR}
    )

target_link_libraries(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  ${TARGET_NAME}
  ${python_LIBRARIES}
  usdGeom
)

set_target_properties(${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  PROPERTIES
    PREFIX "_"
    SUFFIX ".so"
    OUTPUT_NAME ${TARGET_NAME}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}
)

add_subdirectory(tests)

#install python init files
foreach(INPUT_FILE ${PY_INIT_FILES})
  string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_INSTALL_PREFIX}/lib/python/${TARGET_NAME} OUTPUT_FILE ${INPUT_FILE})
  install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${OUTPUT_FILE})")
  install(CODE "execute_process(COMMAND python -m compileall ${OUTPUT_FILE} ${CMAKE_INSTALL_PREFIX})")
endforeach()


# install python library
install(TARGETS ${MAYAUSDUTILS_PYTHON_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${MAYAUSDUTILS_PYTHON_LIBRARY_LOCATION}
)
