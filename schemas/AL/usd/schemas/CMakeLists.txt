set(
    resources_install_path
    ${CMAKE_INSTALL_PREFIX}/share/usd/plugins/${SCHEMAS_PACKAGE}/resources
)

set(
    library_install_path
    ${CMAKE_INSTALL_PREFIX}/lib
)

set(
    bindings_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/python/${SCHEMAS_PYTHON_PATH}
)

# Bake library name
configure_file (
    ./plugInfo.json.in
    ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json
    @ONLY
)

list(APPEND DEPENDANT_LIBRARIES ${PYTHON_LIBRARIES} usd usdGeom)

####################################################################################################
# Schemas generation and validation
####################################################################################################

if(VALIDATE_GENERATED_SCHEMAS)

# Bake library name, tokens prefix and usd schemas path
set(USD_SCHEMA_PATH usd/schema.usda)
set(USDGEOM_SCHEMA_PATH usdGeom/schema.usda)
configure_file (
    ./schema.usda.in
    ./schema.usda
)

execute_process(
    COMMAND
        usdGenSchema
            -v
            ${CMAKE_CURRENT_BINARY_DIR}/schema.usda
            .
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE
        usdgen_res
)
if(usdgen_res)
    message(FATAL_ERROR "Schemas generation failed")
endif()

endif()

####################################################################################################
# Usd plugin
####################################################################################################

# Bake python module path
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/ModuleDeps.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ModuleDeps.cpp"
)

add_library(${SCHEMAS_PACKAGE}
    SHARED
    ModelAPI.cpp
    MayaReference.cpp
    tokens.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ModuleDeps.cpp
)

set_target_properties(${SCHEMAS_PACKAGE}
    PROPERTIES COMPILE_DEFINITIONS 
        "MFB_PACKAGE_NAME=${SCHEMAS_PACKAGE};MFB_ALT_PACKAGE_NAME=${SCHEMAS_PACKAGE};MFB_PACKAGE_MODULE=${SCHEMAS_PYTHON_MODULE}"
)

target_link_libraries(${SCHEMAS_PACKAGE} ${DEPENDANT_LIBRARIES})

install(
    TARGETS ${SCHEMAS_PACKAGE}
    LIBRARY
    DESTINATION ${library_install_path}
)

####################################################################################################
# Python bindings
####################################################################################################

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/module.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/module.cpp"
)

add_library(${SCHEMAS_PYTHON_PACKAGE}
    SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/module.cpp
    wrapModelAPI.cpp
    wrapMayaReference.cpp
    wrapTokens.cpp
)

set_target_properties(${SCHEMAS_PYTHON_PACKAGE}
    PROPERTIES COMPILE_DEFINITIONS
        "MFB_PACKAGE_NAME=${SCHEMAS_PACKAGE};MFB_ALT_PACKAGE_NAME=${SCHEMAS_PACKAGE};MFB_PACKAGE_MODULE=${PYTHON_MODULE}"
)

set_target_properties(${SCHEMAS_PYTHON_PACKAGE} 
    PROPERTIES 
        PREFIX ""
)

target_link_libraries(${SCHEMAS_PYTHON_PACKAGE} ${SCHEMAS_PACKAGE} ${DEPENDANT_LIBRARIES})

install(TARGETS ${SCHEMAS_PYTHON_PACKAGE}
    LIBRARY
    DESTINATION ${bindings_install_path}
)

####################################################################################################
# Setup python packages
####################################################################################################

configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in
  ${CMAKE_BINARY_DIR}/${SCHEMAS_PYTHON_PATH}/__init__.py
)

string(REPLACE "/" ";" folderHierarchy "${SCHEMAS_PYTHON_PATH}")

list(LENGTH folderHierarchy listCount)
math(EXPR listMaxIndex "${listCount}-1")

# Install an __init__ file for all folders except for the terminal folder which uses the checked in
# __init__ file.
if(${listCount} STRGREATER 1)
    set(currentPath "")
    foreach(i RANGE 0 ${listMaxIndex})
      if(${i} STREQUAL ${listMaxIndex})
        break()#we are at the last folder, we will use the checked in __init__.py for this folder.
      endif()
      set(currentFolder "")
      list(GET folderHierarchy ${i} currentFolder)
      list(APPEND currentPath ${currentFolder})

      string(REPLACE ";" "/" currentPath "${currentPath}")
      file(WRITE
        ${CMAKE_BINARY_DIR}/${currentPath}/__init__.py
        "try:\n\t__import__('pkg_resources').declare_namespace(__name__)\nexcept:\n\tfrom pkgutil import extend_path\n\t__path__ = extend_path(__path__, __name__)\n"
      )
    endforeach(i)
endif()

install(
    DIRECTORY
        ${CMAKE_BINARY_DIR}/AL
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/lib/python
)

####################################################################################################
# Install public headers
####################################################################################################

install(
    FILES
        ModelAPI.h
        MayaReference.h
        api.h
        tokens.h
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/AL/usd/schemas
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
    ./plugInfo.json
    .
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${resources_install_path}
)

install(
    FILES generatedSchema.usda 
    DESTINATION ${resources_install_path}
)

####################################################################################################

add_subdirectory(tests)
