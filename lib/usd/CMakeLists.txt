#==============================================================================
# Configuration hdmaya 
#==============================================================================
set(MTOH_CXX_STD "11"
    CACHE
    STRING
    "Which C++ standard / version to use, as an integer - ie, 14 means use c++14."
)

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)
option(INSTALL_MAYA_MOD "Installs a maya module file for the plugin." OFF)
option(HDMAYA_BUILD_TESTS "Build the maya-to-hydra tests" OFF)
option(HDMAYA_UFE_BUILD "Build UFE support for the maya-to-hydra plugin." OFF)
option(BUILD_DOCS "Whether or not to build the documentation." OFF)

# HDMAYA_UFE_BUILD is used in maya-to-hydra plugin for #if test.
# Make sure HDMAYA_UFE_BUILD_INT is an int.
set(HDMAYA_UFE_BUILD_INT 0)

if (NOT "${MAYA_DEVKIT_LOCATION}" STREQUAL "")
    find_package(UFE REQUIRED)
    include_directories(${UFE_INCLUDE_DIR})

    # Currently we control whether or not we want UFE by setting MAYA_DEVKIT_LOCATION
    # It will then force maya-to-hyra to build with UFE.
    set(HDMAYA_UFE_BUILD ON)

    # WANT_UFE_BUILD is used in mayaUsdPlugin for #ifdef test.
    add_definitions(-DWANT_UFE_BUILD)

    # HDMAYA_UFE_BUILD is used in maya-to-hydra plugin for #if test.
    # Make sure HDMAYA_UFE_BUILD_INT is an int.
    set(HDMAYA_UFE_BUILD_INT 1)

    message(STATUS "Found UFE version: ${UFE_VERSION}")
    message(STATUS "UFE Include directory: ${UFE_INCLUDE_DIR}")
    message(STATUS "UFE Library: ${UFE_LIBRARY}")
endif()

add_subdirectory(hdmaya)

add_subdirectory(translators)
add_subdirectory(schemas)
