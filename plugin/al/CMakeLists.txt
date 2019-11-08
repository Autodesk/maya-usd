message("========== AL USD Plugin ==========")

project(AL_USDMaya)

# Disallow use of the LOCATION property for build targets
if (POLICY CMP0026)
    cmake_policy(SET CMP0026 OLD)
endif()

set(NEED_BOOST_FILESYSTEM ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BOOST_ROOT ${PXR_USD_LOCATION})
set(INSTALL_DIR_SUFFIX plugin/al)

option(BUILD_USDMAYA_SCHEMAS "Build optional schemas." ON)
option(BUILD_USDMAYA_TRANSLATORS "Build optional translators." ON)
option(SKIP_USDMAYA_TESTS "Build tests" OFF)

if ("${MAYA_DEVKIT_LOCATION}" STREQUAL "")
    set(CMAKE_WANT_UFE_BUILD OFF)
else()
    set(CMAKE_WANT_UFE_BUILD ON)
endif()

include(${USD_CONFIG_FILE})

find_package(Maya REQUIRED)

# to get PYTHON_EXECUTABLE
find_package(PythonInterp)

# find_package(MayaUSD REQUIRED)

if(CMAKE_WANT_UFE_BUILD)
    add_definitions(-DWANT_UFE_BUILD)

    find_package(UFE REQUIRED)
    include_directories(${UFE_INCLUDE_DIR})

    message("UFE Build Enabled")
    message("Using UFE version : ${UFE_VERSION}")
endif()

# FindBoost is particularly buggy, and doesn't like custom boost locations.
# Adding specific components forces calls to _Boost_find_library, which
# is the rationale for listing them here.
set(Boost_FIND_COMPONENTS
     python
     thread
     filesystem
)
if(WIN32)
    list(APPEND Boost_FIND_COMPONENTS
        chrono
    )
endif()

find_package(Boost COMPONENTS
                ${Boost_FIND_COMPONENTS}
                REQUIRED
)

if(NOT SKIP_USDMAYA_TESTS)
    if (NOT GTEST_FOUND)
        # First see if we can find a gtest that was downloaded and built.
        if (NOT GOOGLETEST_BUILD_ROOT)
            set(GOOGLETEST_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        if (NOT GTEST_ROOT)
            set(GTEST_ROOT "${GOOGLETEST_BUILD_ROOT}/googletest-install")
        endif()
        find_package(GTest QUIET)
        # At this point GTEST_FOUND is set to True in Release but False in Debug.
    endif()

    enable_testing()
    if (NOT GTEST_FOUND)
        #======================================================================
        # Download and unpack googletest at configure time.  Adapted from
        #
        # https://github.com/abseil/googletest/blob/master/googletest/README.md
        #
        # PPT, 22-Nov-2018.

        # Immediately convert CMAKE_MAKE_PROGRAM to forward slashes (if required).
        # Attempting to do so in execute_process fails with string invalid escape
        # sequence parsing errors.  PPT, 22-Nov-2018.
        file(TO_CMAKE_PATH ${CMAKE_MAKE_PROGRAM} CMAKE_MAKE_PROGRAM)

        if (GOOGLETEST_SRC_DIR)
            configure_file(CMakeLists_googletest_src.txt.in ${GOOGLETEST_BUILD_ROOT}/googletest-config/CMakeLists.txt)
        else()
            configure_file(CMakeLists_googletest_download.txt.in ${GOOGLETEST_BUILD_ROOT}/googletest-config/CMakeLists.txt)
        endif()

        message(STATUS "========== Installing GoogleTest... ==========")
        set(FORCE_SHARED_CRT "")
        if(MSVC)
            set(FORCE_SHARED_CRT -DFORCE_SHARED_CRT=OFF)
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} . ${FORCE_SHARED_CRT}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "CMake step for googletest failed: ${result}")
        endif()

        execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "Build step for googletest failed: ${result}")
        endif()
        message(STATUS "========== ...  GoogleTest installed. ==========")

        set(GTEST_ROOT ${GOOGLETEST_BUILD_ROOT}/googletest-install CACHE path "GoogleTest installation root")
        #======================================================================
    endif()

    # https://gitlab.kitware.com/cmake/cmake/issues/17799
    # FindGtest is buggy when dealing with Debug build. 
    if (CMAKE_BUILD_TYPE MATCHES Debug AND GTEST_FOUND MATCHES FALSE)
        message("Setting GTest libraries with debug...")

        if (GTEST_LIBRARY_DEBUG MATCHES GTEST_LIBRARY_DEBUG-NOTFOUND)
                set(gtest_library "")
                set(gtest_main_library "")
            if(WIN32)
                set(gtest_library lib/gtestd.lib)
                set(gtest_main_library lib/gtest_maind.lib)
            else()
                set(gtest_library lib64/libgtestd.a)
                set(gtest_main_library lib64/libgtest_maind.a)
            endif()
                set(GTEST_INCLUDE_DIRS ${GOOGLETEST_BUILD_ROOT}/googletest-install/include)
                set(GTEST_LIBRARY_DEBUG ${GOOGLETEST_BUILD_ROOT}/googletest-install/${gtest_library})
                set(GTEST_MAIN_LIBRARY_DEBUG ${GOOGLETEST_BUILD_ROOT}/googletest-install/${gtest_main_library})
        endif()

        set(GTEST_LIBRARY ${GTEST_LIBRARY_DEBUG})
        set(GTEST_LIBRARIES ${GTEST_LIBRARY})
        set(GTEST_MAIN_LIBRARY ${GTEST_MAIN_LIBRARY_DEBUG})
        set(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARY})
    endif()
endif()

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
set(MAYATEST_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/mayatest)
set(USDTRANSACTION_INCLUDE_LOCATION ${CMAKE_CURRENT_LIST_DIR}/usdtransaction)

add_subdirectory(utils)
add_subdirectory(usdutils)
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

install(CODE "message(STATUS \"POST INSTALL: Compiling python/pyc for ${CMAKE_INSTALL_PREFIX} ... \")")
install(CODE "execute_process(COMMAND python -m compileall ${CMAKE_INSTALL_PREFIX} )")
