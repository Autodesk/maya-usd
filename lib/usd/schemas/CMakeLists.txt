set(SCHEMA_PLUGIN mayaUsd_Schemas)

set(
    resources_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${SCHEMA_PLUGIN}/resources
)

set(
    library_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/
)

# Bake library name
configure_file (
    plugInfo.json.in
    ./plugInfo.json
    @ONLY
)

list(APPEND DEPENDANT_LIBRARIES ${PYTHON_LIBRARIES} usd usdGeom)

####################################################################################################
# Schemas generation
####################################################################################################

# Bake library name, tokens prefix and usd schemas path
set(USD_SCHEMA_PATH usd/schema.usda)
set(USDGEOM_SCHEMA_PATH usdGeom/schema.usda)
configure_file (
    ./schema.usda.in
    ./schema.usda
)

execute_process(
    COMMAND
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

####################################################################################################
# Usd plugin
####################################################################################################

add_library(${SCHEMA_PLUGIN}
    SHARED
    ALMayaReference.cpp
    MayaReference.cpp
    ModuleDeps.cpp
    tokens.cpp
)

# Before 0.19.11, pxrConfig.cmake did not properly add the USD_ROOT/include dir
# the INTERFACE_INCLUDE_DIRECTORIES of it's created targets...
if(USD_VERSION VERSION_LESS 0.19.11)
target_include_directories(${SCHEMA_PLUGIN}
  PUBLIC
    ${USD_INCLUDE_DIR}
)
endif()

# Hamed 2019 
# https://stackoverflow.com/questions/25617839/undefined-reference-to-symbol-pthread-key-deleteglibc-2-2-5
set(PTHREAD_LINK "")
if(IS_LINUX)
    set(PTHREAD_LINK -lpthread -lm)
endif()

#======================================================================
# TARGET PROPERTIES
#======================================================================
if(IS_MACOSX OR IS_LINUX)
    # rpath setup
    mayaUsd_init_rpath(rpath "plugin")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${SCHEMA_PLUGIN})
endif()

if(IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../USD/lib64")
    endif()
endif()

target_include_directories( ${SCHEMA_PLUGIN} PUBLIC
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(${SCHEMA_PLUGIN} ${DEPENDANT_LIBRARIES} ${PTHREAD_LINK})

set(mayaUsdSchemas_headers
    api.h
    ALMayaReference.h
    MayaReference.h
    tokens.h
)
mayaUsd_promoteHeaderListWithSubdir(
    HEADERS ${mayaUsdSchemas_headers}
    BASE_PATH "mayaUsd_Schemas" 
)

install(
    TARGETS ${SCHEMA_PLUGIN}
    LIBRARY
    DESTINATION ${library_install_path}
	ARCHIVE
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${SCHEMA_PLUGIN}> DESTINATION ${library_install_path} OPTIONAL)
endif()

####################################################################################################
# Python bindings
####################################################################################################

set(SCHEMAS_LIBRARY_NAME schemas)

if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(SCHEMAS_PYTHON_LIBRARY_NAME "_${SCHEMAS_LIBRARY_NAME}_d")
else()
    set(SCHEMAS_PYTHON_LIBRARY_NAME "_${SCHEMAS_LIBRARY_NAME}")
endif()

add_library(${SCHEMAS_PYTHON_LIBRARY_NAME}
    SHARED
    module.cpp
    wrapTokens.cpp
    wrapMayaReference.cpp
    wrapALMayaReference.cpp
)
    
target_compile_definitions(${SCHEMAS_PYTHON_LIBRARY_NAME}
    PRIVATE
        MFB_PACKAGE_NAME=${SCHEMAS_LIBRARY_NAME}
        MFB_ALT_PACKAGE_NAME=${SCHEMAS_LIBRARY_NAME}
        MFB_PACKAGE_MODULE=mayaUsd.schemas
)

set_target_properties(${SCHEMAS_PYTHON_LIBRARY_NAME} 
    PROPERTIES 
        PREFIX ""
)

set_python_module_suffix(${SCHEMAS_PYTHON_LIBRARY_NAME})

target_link_libraries(${SCHEMAS_PYTHON_LIBRARY_NAME}
    ${SCHEMA_PLUGIN}
)

if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "${PROJECT_NAME}")
    mayaUsd_add_rpath(rpath "../../../")
    mayaUsd_install_rpath(rpath ${SCHEMAS_PYTHON_LIBRARY_NAME})
endif()

install(TARGETS ${SCHEMAS_PYTHON_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${SCHEMAS_LIBRARY_NAME}
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${SCHEMAS_LIBRARY_NAME}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${SCHEMAS_PYTHON_LIBRARY_NAME}> 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${SCHEMAS_LIBRARY_NAME} OPTIONAL)
endif()

install(FILES __init__.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/${SCHEMAS_LIBRARY_NAME}
)

####################################################################################################
# Install public headers
####################################################################################################

install(
    FILES
        ${mayaUsdSchemas_headers}
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/mayaUsd_Schemas/
)

####################################################################################################
# Install usd plugin resources
####################################################################################################

file(RELATIVE_PATH
    SCHEMAS_LIBRARY_DIR
    ${resources_install_path}/..
    ${library_install_path}

)

# Bake relative path
configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    .
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${resources_install_path}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generatedSchema.usda 
    DESTINATION ${resources_install_path}
)

