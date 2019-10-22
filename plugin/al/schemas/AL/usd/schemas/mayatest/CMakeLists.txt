set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX} )

set(
    resources_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/usd/AL_USDMayaSchemasTest/resources
)

set(
    library_install_path
    ${CMAKE_INSTALL_PREFIX}/lib
)

set(
    bindings_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/python/AL/usd/schemas/mayatest
)

# Bake library name
configure_file (
    plugInfo.json.in
    ./plugInfo.json
    @ONLY
)

list(APPEND DEPENDANT_LIBRARIES ${PYTHON_LIBRARIES} usd)

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

add_library(AL_USDMayaSchemasTest
    SHARED
    ModuleDeps.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ExamplePolyCubeNode.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp
)

target_compile_definitions(AL_USDMayaSchemasTest
    PRIVATE
        MFB_PACKAGE_NAME=AL_USDMayaSchemasTest
        MFB_ALT_PACKAGE_NAME=AL_USDMayaSchemasTest
        MFB_PACKAGE_MODULE=AL.usd.schemas.mayatest
        AL_USDMAYASCHEMASTEST_EXPORTS
)

target_include_directories(AL_USDMayaSchemasTest 
  PUBLIC
    ${USD_INCLUDE_DIR}
)

target_link_libraries(AL_USDMayaSchemasTest ${DEPENDANT_LIBRARIES})

install(
    TARGETS AL_USDMayaSchemasTest
    LIBRARY
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:AL_USDMayaSchemasTest> DESTINATION ${library_install_path} OPTIONAL)
endif()

####################################################################################################
# Python bindings
####################################################################################################

add_library(_AL_USDMayaSchemasTest
    SHARED
    module.cpp
    wrapTokens.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/wrapExamplePolyCubeNode.cpp
)

target_compile_definitions(_AL_USDMayaSchemasTest
    PRIVATE
        MFB_PACKAGE_NAME=AL_USDMayaSchemasTest
        MFB_ALT_PACKAGE_NAME=AL_USDMayaSchemasTest
        MFB_PACKAGE_MODULE=AL.usd.schemas.mayatest
)

set_target_properties(_AL_USDMayaSchemasTest
    PROPERTIES 
        PREFIX ""
)

if(MSVC)
set_target_properties(_AL_USDMayaSchemasTest
    PROPERTIES
        SUFFIX ".pyd"
)
endif()

target_link_libraries(_AL_USDMayaSchemasTest AL_USDMayaSchemasTest ${DEPENDANT_LIBRARIES})

install(TARGETS _AL_USDMayaSchemasTest
    LIBRARY
    DESTINATION ${bindings_install_path}
    RUNTIME
    DESTINATION ${bindings_install_path}
)

####################################################################################################
# Setup python packages
####################################################################################################

# configure_file has a nice feature where it will copy the __init__ file over when it gets modified, unlike file(COPY ...)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py ${CMAKE_BINARY_DIR}/AL/usd/schemas/mayatest/__init__.py COPYONLY)
install(CODE "execute_process(COMMAND python -m compileall ${CMAKE_BINARY_DIR}/AL/usd/schemas/mayatest/__init__.py )")

string(REPLACE "/" ";" folderHierarchy "AL/usd/schemas/mayatest")

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

install(CODE "message(STATUS \"POST INSTALL: Compiling python/pyc for ${CMAKE_INSTALL_PREFIX}/lib/python ... \")")
install(CODE "execute_process(COMMAND python -m compileall ${CMAKE_INSTALL_PREFIX}/lib/python )")

####################################################################################################
# Install usd plugin resources
####################################################################################################

file(RELATIVE_PATH
    SCHEMASTEST_LIBRARY_DIR
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

