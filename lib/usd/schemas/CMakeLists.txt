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
        ${PYTHON_EXECUTABLE} 
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
    MayaReference.cpp
    tokens.cpp
# Python wrappers are not yet defined for this library.  See
# plugin/al/schemas/AL/usd/schemas/maya/CMakeLists.txt
# for the existing AL schema Python wrappers.  PPT, 26-Jul-2019.
#    ModuleDeps.cpp
)

target_compile_definitions(${SCHEMA_PLUGIN}
    PRIVATE
        MFB_PACKAGE_NAME=${SCHEMA_PLUGIN}
        MFB_ALT_PACKAGE_NAME=${SCHEMA_PLUGIN}
# Python wrappers are not yet defined for this library.  PPT, 26-Jul-2019.
#        MFB_PACKAGE_MODULE=AL.usd.schemas.maya
        MAYAUSDCORE_SCHEMAS_EXPORTS
)

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
    init_rpath(rpath "plugin")
    if(DEFINED WANT_USD_RELATIVE_PATH)
        add_rpath(rpath "../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    install_rpath(rpath ${SCHEMA_PLUGIN})
endif()

if(IS_LINUX)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags" PARENT_SCOPE)
endif()

target_link_libraries(${SCHEMA_PLUGIN} ${DEPENDANT_LIBRARIES} ${PTHREAD_LINK})

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
# Install public headers
####################################################################################################

install(
    FILES
        MayaReference.h
        api.h
        tokens.h
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/schemas
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

