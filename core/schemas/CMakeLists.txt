set(
    resources_install_path
    ${CMAKE_INSTALL_PREFIX}/plugin/usd/mayaUsdCore_Schemas/resources
)

set(
    library_install_path
    ${CMAKE_INSTALL_PREFIX}/plugin/usd/
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
        python
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

add_library(MayaUsdCore_Schemas
    SHARED
    MayaReference.cpp
    tokens.cpp
    ModuleDeps.cpp
)

target_compile_definitions(MayaUsdCore_Schemas
    PRIVATE
        MFB_PACKAGE_NAME=MayaUsdCore_Schemas
        MFB_ALT_PACKAGE_NAME=MayaUsdCore_Schemas
        MFB_PACKAGE_MODULE=AL.usd.schemas.maya
        MAYAUSD_CORE_SCHEMAS_EXPORTS
)

# Hamed 2019 
# https://stackoverflow.com/questions/25617839/undefined-reference-to-symbol-pthread-key-deleteglibc-2-2-5
set(PTHREAD_LINK "")
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PTHREAD_LINK -lpthread -lm)
endif()

target_link_libraries(MayaUsdCore_Schemas ${DEPENDANT_LIBRARIES} ${PTHREAD_LINK})

install(
    TARGETS MayaUsdCore_Schemas
    LIBRARY
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:MayaUsdCore_Schemas> DESTINATION ${library_install_path} OPTIONAL)
endif()

####################################################################################################
# Install public headers
####################################################################################################

install(
    FILES
        MayaReference.h
        api.h
        tokens.h
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/mayaUsdCore/schemas
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

