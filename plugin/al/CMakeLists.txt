message("========== AL USD Plugin ==========")

project(AL_USDMaya)

# Disallow use of the LOCATION property for build targets
if (POLICY CMP0026)
    cmake_policy(SET CMP0026 OLD)
endif()

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(INSTALL_DIR_SUFFIX plugin/al)
set(AL_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX})

option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_USDMAYA_TRANSLATORS "Build optional translators." ON)
option(SKIP_USDMAYA_TESTS "Build tests" OFF)

set(AL_USDMAYA_LOCATION_NAME
    "AL_USDMAYA_LOCATION"
    CACHE
    STRING
    "Name of the environment variable used to store AL_USDMaya installation location"
)

# Build all the utils
set(EVENTS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/utils)
set(MAYAUTILS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/mayautils)
set(USDMAYAUTILS_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/usdmayautils)
set(MAYATEST_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/mayatest)
set(USDTRANSACTION_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/usdtransaction)

add_subdirectory(utils)
if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(mayatest)
endif() 
add_subdirectory(mayautils)
add_subdirectory(usdmayautils)
add_subdirectory(usdtransaction)

add_subdirectory(lib)
add_subdirectory(plugin)

if(BUILD_USDMAYA_SCHEMAS)
    message(STATUS "Building AL_USDMayaSchemas")
    add_subdirectory(schemas)
    if(BUILD_USDMAYA_TRANSLATORS)
        message(STATUS "Building AL_USDMayaTranslators")
        set(USD_MAYA_INCLUDE_DIR ${CMAKE_BINARY_DIR}/maya/include)
        set(TRANSLATORS_PACKAGE AL_USDMayaTranslators)
        add_subdirectory(translators)
    endif()
endif()

get_property(LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION)
get_property(PYTHON_LIBRARY_LOCATION GLOBAL PROPERTY GLOBAL_PYTHON_LIBRARY_LOCATION)
configure_file(ALUsdMayaConfig.cmake.in ${PROJECT_BINARY_DIR}/ALUsdMayaConfig.cmake @ONLY)
