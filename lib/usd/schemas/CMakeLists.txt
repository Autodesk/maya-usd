set(TARGET_NAME mayaUsd_Schemas)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        ALMayaReference.cpp
        MayaReference.cpp
        ModuleDeps.cpp
        tokens.cpp
)

set(HEADERS
    api.h
    ALMayaReference.h
    MayaReference.h
    tokens.h
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PUBLIC
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS}
                          BASEDIR ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME} 
    PUBLIC
        usd 
        usdGeom
        ${PYTHON_LIBRARIES} 
        ${PTHREAD_LINK}
        $<$<BOOL:${IS_LINUX}>:"pthread">
        $<$<BOOL:${IS_LINUX}>:"m">
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
file(RELATIVE_PATH
    SCHEMAS_LIBRARY_DIR
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources/..
    ${CMAKE_INSTALL_PREFIX}/lib/
)

configure_file("plugInfo.json.in" "./plugInfo.json" @ONLY)
configure_file("${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json" .) 

install(
    TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

install(FILES ${HEADERS}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generatedSchema.usda 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources
)

#------------------------------------------------------------------------------
# Schemas generation
#------------------------------------------------------------------------------
# Bake library name, tokens prefix and usd schemas path
set(USD_SCHEMA_PATH usd/schema.usda)
set(USDGEOM_SCHEMA_PATH usdGeom/schema.usda)
configure_file (./schema.usda.in ./schema.usda)
execute_process(COMMAND
                    ${Python_EXECUTABLE}
                    ${USD_GENSCHEMA}
                    ${CMAKE_CURRENT_BINARY_DIR}/schema.usda
                    .
                WORKING_DIRECTORY
                    ${CMAKE_CURRENT_BINARY_DIR}
                RESULT_VARIABLE
                    usdgen_res
)
if(usdgen_res)
    message(FATAL_ERROR "Schemas generation failed")
endif()

#------------------------------------------------------------------------------
# python bindings
#------------------------------------------------------------------------------
set(SCHEMAS_LIBRARY_NAME schemas)

if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(TARGET_PYTHON_NAME "_${SCHEMAS_LIBRARY_NAME}_d")
else()
    set(TARGET_PYTHON_NAME "_${SCHEMAS_LIBRARY_NAME}")
endif()

add_library(${TARGET_PYTHON_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_PYTHON_NAME} 
    PRIVATE
        module.cpp
        wrapTokens.cpp
        wrapMayaReference.cpp
        wrapALMayaReference.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_PYTHON_NAME}
    PRIVATE
        MFB_PACKAGE_NAME=${SCHEMAS_LIBRARY_NAME}
        MFB_ALT_PACKAGE_NAME=${SCHEMAS_LIBRARY_NAME}
        MFB_PACKAGE_MODULE=mayaUsd.schemas
)

mayaUsd_compile_config(${TARGET_PYTHON_NAME})

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
set_python_module_property(${TARGET_PYTHON_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_PYTHON_NAME}
    PUBLIC
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath mayaUsd)
    mayaUsd_add_rpath(rpath "../../../")
    mayaUsd_install_rpath(rpath ${TARGET_PYTHON_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_PYTHON_NAME}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/mayaUsd/${SCHEMAS_LIBRARY_NAME}
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/mayaUsd/${SCHEMAS_LIBRARY_NAME}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_PYTHON_NAME}> 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/mayaUsd/${SCHEMAS_LIBRARY_NAME} OPTIONAL)
endif()

install(FILES __init__.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/mayaUsd/${SCHEMAS_LIBRARY_NAME}
)
