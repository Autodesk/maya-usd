#This is a modified version of the cmake file from usd/sdk/maya/CMakeLists.txt 
cmake_minimum_required(VERSION 2.8.12)
project(AL_USDMaya)

#set cmake modules - ";" separated not ":"!
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# AL_USDMAYA uses pxrConfig.cmake exported targets
# find_package(USD) can use either ${USD_ROOT}, or ${USD_CONFIG_FILE} (which should be ${USD_ROOT}/pxrConfig.cmake)
# to find USD, defined as either Cmake var or env var
find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

# to get PYTHON_EXECUTABLE
find_package(PythonInterp)

include(ProjectDefaults)
include(Public)
include(CXXDefaults)

add_definitions(${_PXR_CXX_DEFINITIONS})

find_package(Maya REQUIRED)

include_directories(${MAYA_INCLUDE_DIRS})

if(${MAYA_API_VERSION} STRGREATER 2017)
    find_package(Qt5Gui REQUIRED)
else()
    find_package(Qt4 REQUIRED QtGui)
endif()

include_directories(${PXR_INCLUDE_DIRS})

# FindBoost is particularly buggy, and doesn't like custom boost locations.
# Adding specific components forces calls to _Boost_find_library, which
# is the rationale for listing them here.
find_package(Boost COMPONENTS
                python
                thread
                REQUIRED
)

set(CMAKE_CXX_FLAGS
    -std=c++11
    -msse3
    "${_PXR_CXX_WARNING_FLAGS}"
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_VERBOSE_MAKEFILE OFF)

option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_USDMAYA_TRANSLATORS "Build optional translators." ON)
option(VALIDATE_GENERATED_SCHEMAS "Generate schemas code and compare." OFF)

set(AL_USDMAYA_LOCATION_NAME
    "AL_USDMAYA_LOCATION"
    CACHE
    STRING
    "Name of the environment variable used to store AL_USDMaya installation location"
)

# Build all the utils
set(EVENTS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/utils)
set(USDUTILS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/usdutils)
set(MAYAUTILS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/mayautils)
set(USDMAYAUTILS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/usdmayautils)

add_subdirectory(utils)
add_subdirectory(usdutils)
add_subdirectory(mayautils)
add_subdirectory(usdmayautils)

add_subdirectory(lib)
add_subdirectory(plugin)

if(BUILD_USDMAYA_SCHEMAS)
    message(STATUS "Building AL_USDMayaSchemas")
    set(SCHEMAS_PYTHON_MODULE AL.usd.maya.schemas)
    set(SCHEMAS_PYTHON_PATH AL/usd/maya/schemas)
    set(SCHEMAS_PACKAGE AL_USDMayaSchemas)
    set(SCHEMAS_PYTHON_PACKAGE _AL_USDMayaSchemas)
    set(SCHEMAS_TOKENS_PREFIX AL_USDMayaSchemas)
    add_subdirectory(schemas)
    if(BUILD_USDMAYA_TRANSLATORS)
        message(STATUS "Building AL_USDMayaTranslators")
        set(TRANSLATORS_PACKAGE AL_USDMayaTranslators)
        add_subdirectory(translators)
    endif()
endif()

get_property(LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION)
get_property(PYTHON_LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_PYTHON_LIBRARY_LOCATION)
configure_file(ALUsdMayaConfig.cmake.in ${PROJECT_BINARY_DIR}/ALUsdMayaConfig.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/ALUsdMayaConfig.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  COMPONENT dev
)
