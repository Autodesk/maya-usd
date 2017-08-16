if(NOT USD_SCHEMA_PATH)
    # PXR_CMAKE_DIR should be defined in pxrConfig.cmake
    set(USD_SCHEMA_PATH ${PXR_CMAKE_DIR}/share/usd/plugins/usd/resources/schema.usda)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/schema.usda.in"
  "${CMAKE_CURRENT_BINARY_DIR}/schema.usda"
)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
  @ONLY
)

execute_process(
  COMMAND
    usdGenSchema ${CMAKE_CURRENT_BINARY_DIR}/schema.usda ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

list(APPEND DEPENDANT_LIBRARIES ${PYTHON_LIBRARIES} usd )

####################################################################################################
# Usd plugin
####################################################################################################

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/ModuleDeps.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ModuleDeps.cpp"
)

add_library(${SCHEMAS_PACKAGE}
    SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/MayaReference.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/HostDrivenTransformInfo.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp
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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

# Copy the generated files resources in a hierarchy replicating the insallation hierarchy.
# TODO: use install prefix variable.
get_target_property( LIBRARY_LOCATION
  ${SCHEMAS_PACKAGE}
  LOCATION
  )

get_filename_component(LIBRARY_LOCATION
  ${LIBRARY_LOCATION}
  DIRECTORY
  )

configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    ${CMAKE_CURRENT_BINARY_DIR}/share/usd/plugins/${SCHEMAS_PACKAGE}/resources/plugInfo.json
    )

file(
    COPY
    ${CMAKE_CURRENT_BINARY_DIR}/generatedSchema.usda
    DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/share/usd/plugins/${SCHEMAS_PACKAGE}/resources
    )

# Setup the actual plugInfo.json.
set(LIBRARY_LOCATION "../../../../lib")
configure_file(
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    )


configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/module.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/module.cpp"
)

####################################################################################################
# Python bindings
####################################################################################################

add_library(${SCHEMAS_PYTHON_PACKAGE}
    SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/module.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/wrapMayaReference.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/wrapHostDrivenTransformInfo.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/wrapTokens.cpp
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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${SCHEMAS_PYTHON_PATH}
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
        "try:\n\t__import__('pkg_resources').declare_namespace(__name__)\nexcept:\n\tpass\n"
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
        ${CMAKE_CURRENT_BINARY_DIR}/MayaReference.h
        ${CMAKE_CURRENT_BINARY_DIR}/HostDrivenTransformInfo.h
        ${CMAKE_CURRENT_BINARY_DIR}/api.h
        ${CMAKE_CURRENT_BINARY_DIR}/tokens.h
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/AL/usd/schemas
)

####################################################################################################
# Install usd plugin resources
####################################################################################################

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/usd/plugins/${SCHEMAS_PACKAGE}/resources
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generatedSchema.usda 
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/usd/plugins/${SCHEMAS_PACKAGE}/resources
)

####################################################################################################

add_subdirectory(tests)
