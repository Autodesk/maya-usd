#This is a modified version of the cmake file from usd/sdk/maya/CMakeLists.txt 
cmake_minimum_required(VERSION 2.8.12)
project(AL_USDMaya)

# AL_USDMAYA uses pxrConfig.cmake exported targets
# Grab USD_CONFIG_FILE either as a cmake define or an environment variable
if(NOT USD_CONFIG_FILE)
    if(NOT DEFINED ENV{USD_CONFIG_FILE})
        message(FATAL_ERROR "Please set USD_CONFIG_FILE to the path to pxrConfig.cmake")
    else()
        set(USD_CONFIG_FILE $ENV{USD_CONFIG_FILE})
    endif()
endif()
if(NOT EXISTS ${USD_CONFIG_FILE})
    message( FATAL_ERROR "pxrConfig.cmake is needed, please set USD_CONFIG_FILE to a valid path")
else()
    include(${USD_CONFIG_FILE})
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

#set cmake modules - ";" separated not ":"!
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# to get PYTHON_EXECUTABLE
find_package(PythonInterp)

include(ProjectDefaults)
include(Public)
include(CXXDefaults)

add_definitions(${_PXR_CXX_DEFINITIONS})

find_package(Maya REQUIRED)
include_directories(${MAYA_INCLUDE_DIRS})

find_package(Qt5Gui REQUIRED)

include_directories(${PXR_INCLUDE_DIRS})

# FindBoost is particularly buggy, and doesn't like custom boost locations.
# Adding specific components forces calls to _Boost_find_library, which
# is the rationale for listing them here.
find_package(Boost COMPONENTS
                python
                REQUIRED
)

set(CMAKE_CXX_FLAGS
    -std=c++11
    -msse3
    "${_PXR_CXX_WARNING_FLAGS}"
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_VERBOSE_MAKEFILE OFF)

# copy pxr_setup_python to add namespace package definition
function(al_setup_python)
    get_property(pxrPythonModules GLOBAL PROPERTY PXR_PYTHON_MODULES)

    # A new list where each python module is quoted
    set(converted "")
    foreach(module ${pxrPythonModules})
        list(APPEND converted "'${module}'")
    endforeach()

    # Join these with a ', '
    string(REPLACE ";" ", " pyModulesStr "${converted}")

    # Install a pxr __init__.py with an appropriate __all__
    _get_install_dir(lib/python/pxr installPrefix)
    install(CODE
        "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${installPrefix}/__init__.py\" \"try:\n\t__import__('pkg_resources').declare_namespace(__name__)\nexcept:\n\tpass\nfrom pkgutil import extend_path\n__path__ = extend_path(__path__, __name__)\n__all__ = [${pyModulesStr}]\n\")"
    )

endfunction()

option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_USDMAYA_TRANSLATORS "Build optional translators." ON)

# Build our plugs... non-standard as we're building from another source tree
add_subdirectory(lib)
add_subdirectory(plugin)
if(BUILD_USDMAYA_SCHEMAS)
    message(STATUS "Building AL_USDMayaSchemas")
    set(SCHEMAS_PYTHON_MODULE AL.usd.maya.schemas)
    set(SCHEMAS_PYTHON_PATH AL/usd/maya/schemas)
    set(SCHEMAS_PACKAGE AL_USDMayaSchemas)
    set(SCHEMAS_PYTHON_PACKAGE _AL_USDMayaSchemas)
    set(SCHEMAS_TOKENS_PREFIX AL_MayaSchemas)
    add_subdirectory(schemas)
endif()
if(BUILD_USDMAYA_TRANSLATORS)
    message(STATUS "Building AL_USDMayaTranslators")
    set(TRANSLATORS_PACKAGE AL_USDMayaTranslators)
    add_subdirectory(translators)
endif()
add_subdirectory(tests)

al_setup_python()
pxr_setup_plugins()

get_property(LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION)
get_property(PYTHON_LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_PYTHON_LIBRARY_LOCATION)
configure_file(ALUsdMayaConfig.cmake.in ${PROJECT_BINARY_DIR}/ALUsdMayaConfig.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/ALUsdMayaConfig.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  COMPONENT dev
)
