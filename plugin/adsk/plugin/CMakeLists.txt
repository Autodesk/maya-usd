set(PLUGIN_PACKAGE mayaUsdPlugin)

#==============================================================================
# Configuration for mayaUsdPlugin and maya-to-hydra Plugin
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
option(BUILD_DOCS "Wether to build the Documentation." OFF)

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

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
    importTranslator.cpp
)

target_compile_definitions(${PLUGIN_PACKAGE}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
)

target_include_directories(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
		${PXR_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}
		ar 
		gf 
		kind
		plug 
		sdf 
		tf
		usd
		usdGeom
		usdUtils
)

if(MSVC)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set_target_properties(${PLUGIN_PACKAGE}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../lib")
else()
    set(rpath "$ORIGIN:$ORIGIN/../lib")
    if (CMAKE_INSTALL_RPATH)
        set(rpath "${CMAKE_INSTALL_RPATH}:${rpath}")
    endif()
    set_target_properties(${PLUGIN_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH "${rpath}"
    )
endif()

install(TARGETS
    ${PLUGIN_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()

add_subdirectory(renderers/maya-to-hydra/hdmaya)
add_subdirectory(renderers/maya-to-hydra/plugin)
