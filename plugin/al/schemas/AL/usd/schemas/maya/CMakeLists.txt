set(
    resources_install_path
    ${AL_INSTALL_PREFIX}/lib/usd/AL_USDMayaSchemas/resources
)

set(
    library_install_path
    ${AL_INSTALL_PREFIX}/lib
)

set(
    bindings_install_path
    ${AL_INSTALL_PREFIX}/lib/python/AL/usd/schemas/maya
)

# Bake library name
configure_file (
    plugInfo.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
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
        "${Python_EXECUTABLE}"
        "${USD_GENSCHEMA}"
        "${CMAKE_CURRENT_BINARY_DIR}/schema.usda"
        .
    WORKING_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}"
    RESULT_VARIABLE
        usdgen_res
)
if(usdgen_res)
    message(FATAL_ERROR "Schemas generation failed")
endif()

####################################################################################################
# Usd plugin
####################################################################################################

add_library(AL_USDMayaSchemas
    SHARED
    ModelAPI.cpp
    FrameRange.cpp
    tokens.cpp
    ModuleDeps.cpp
)

# compiler configuration
mayaUsd_compile_config(AL_USDMayaSchemas)

target_compile_definitions(AL_USDMayaSchemas
    PRIVATE
        AL_USDMAYASCHEMAS_EXPORTS
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

target_include_directories(AL_USDMayaSchemas 
  PUBLIC
    ${USD_INCLUDE_DIR}
)

# Hamed 2019 
# https://stackoverflow.com/questions/25617839/undefined-reference-to-symbol-pthread-key-deleteglibc-2-2-5
set(PTHREAD_LINK "")
if(IS_LINUX)
    set(PTHREAD_LINK -lpthread -lm)
endif()

target_link_libraries(AL_USDMayaSchemas ${DEPENDANT_LIBRARIES} ${PTHREAD_LINK})

install(
    TARGETS AL_USDMayaSchemas
    LIBRARY
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:AL_USDMayaSchemas> DESTINATION ${library_install_path} OPTIONAL)
endif()

####################################################################################################
# Python bindings
####################################################################################################

if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(PYTHON_LIBRARY_NAME _AL_USDMayaSchemas_d)
else()
    set(PYTHON_LIBRARY_NAME _AL_USDMayaSchemas)
endif()

add_library(${PYTHON_LIBRARY_NAME}
    SHARED
    module.cpp
    wrapModelAPI.cpp
    wrapTokens.cpp
    wrapFrameRange.cpp
)

# compiler configuration
mayaUsd_compile_config(${PYTHON_LIBRARY_NAME})

target_compile_definitions(${PYTHON_LIBRARY_NAME}
    PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        MFB_PACKAGE_NAME=AL_USDMayaSchemas
        MFB_ALT_PACKAGE_NAME=AL_USDMayaSchemas
        MFB_PACKAGE_MODULE=AL.usd.schemas.maya
)

set_python_module_property(${PYTHON_LIBRARY_NAME})

target_link_libraries(${PYTHON_LIBRARY_NAME} AL_USDMayaSchemas ${DEPENDANT_LIBRARIES})

install(TARGETS ${PYTHON_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${bindings_install_path}
    RUNTIME
    DESTINATION ${bindings_install_path}
)

####################################################################################################
# Setup python packages
####################################################################################################

# configure_file has a nice feature where it will copy the __init__ file over when it gets modified, unlike file(COPY ...)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py ${CMAKE_BINARY_DIR}/AL/usd/schemas/maya/__init__.py COPYONLY)

string(REPLACE "/" ";" folderHierarchy "AL/usd/schemas/maya")

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
        ${AL_INSTALL_PREFIX}/lib/python
)

####################################################################################################
# Install public headers
####################################################################################################

install(
    FILES
        ModelAPI.h
        FrameRange.h
        api.h
        tokens.h
    DESTINATION
        ${AL_INSTALL_PREFIX}/include/AL/usd/schemas/maya
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
    ${CMAKE_CURRENT_BINARY_DIR}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${resources_install_path}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generatedSchema.usda 
    DESTINATION ${resources_install_path}
)

####################################################################################################

if(NOT SKIP_USDMAYA_TESTS)
    add_subdirectory(tests)
endif()
