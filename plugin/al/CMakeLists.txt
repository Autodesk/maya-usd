message("========== AL USD Plugin ==========")

project(AL_USDMaya)

# Disallow use of the LOCATION property for build targets
if (POLICY CMP0026)
    cmake_policy(SET CMP0026 OLD)
endif()

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(NOT DEFINED ENV{BOOST_ROOT})
    set(ENV{BOOST_ROOT} ${PXR_USD_LOCATION})
endif()

set(INSTALL_DIR_SUFFIX plugin/al)
set(AL_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX})

option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_USDMAYA_TRANSLATORS "Build optional translators." ON)
option(SKIP_USDMAYA_TESTS "Build tests" OFF)

if(WIN32)
    if(MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
        set(Boost_USE_DEBUG_PYTHON ON)
    endif()
endif()

find_package(Boost REQUIRED)

# As of boost 1.67 the boost_python component name includes the
# associated Python version (e.g. python27, python36). After boost 1.70
# the built-in cmake files will deal with this. If we are using boost
# that does not have working cmake files, or we are using a new boost
# and not using cmake's boost files, we need to do the below.
# https://cmake.org/cmake/help/latest/module/FindBoost.html
# Find the component under the versioned name and then set the generic
# Boost_PYTHON_LIBRARY variable so that we don't have to duplicate this
# logic in each library's CMakeLists.txt.
set(boost_version_string "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
if (((${boost_version_string} VERSION_GREATER_EQUAL "1.67") AND
    (${boost_version_string} VERSION_LESS "1.70")) OR
    ((${boost_version_string} VERSION_GREATER_EQUAL "1.70") AND
    Boost_NO_BOOST_CMAKE))

    set(python_version_nodot "${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
    set(Boost_FIND_COMPONENTS
        python${python_version_nodot}
    )

    set(Boost_PYTHON_LIBRARY "${Boost_PYTHON${python_version_nodot}_LIBRARY}")
else()
    set(Boost_FIND_COMPONENTS
        python
    )
endif()

find_package(Boost COMPONENTS
                ${Boost_FIND_COMPONENTS}
                REQUIRED
)

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
