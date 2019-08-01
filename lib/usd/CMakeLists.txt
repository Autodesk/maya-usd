#==============================================================================
# Configuration hdMaya 
#==============================================================================
set(MTOH_CXX_STD "11"
    CACHE
    STRING
    "Which C++ standard / version to use, as an integer - ie, 14 means use c++14."
)

option(LUMA_USD_BUILD "Build the plugin for Luma's version of USD." OFF)
option(INSTALL_MAYA_MOD "Installs a maya module file for the plugin." OFF)
option(HDMAYA_BUILD_TESTS "Build the maya-to-hydra tests" OFF)
option(BUILD_DOCS "Whether or not to build the documentation." OFF)

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

add_subdirectory(hdMaya)

# Adds the proper RPATH (on Mac OS) or RUNPATH (on Linux) for PlugPlugin
# translator plugins.  RUNPATH is preferable to RPATH on Linux, as it is
# overridable with LD_LIBRARY_PATH.  There is no RUNPATH on Mac OS.
function(set_translators_plugin_properties plugin)
    # ${PXR_USD_LOCATION}/lib is added to the RPATH only if PXR_USD_LOCATION
    # is set.
    if(APPLE)
    	set(TARGET_RPATH "@loader_path/../lib;@loader_path/../../USD/lib")
    	if(DEFINED PXR_USD_LOCATION)
    	    string(APPEND TARGET_RPATH ";${PXR_USD_LOCATION}/lib")
        endif()
        set(CMAKE_MACOSX_RPATH ON)
        set_target_properties(${plugin}
            PROPERTIES
                SUFFIX ".bundle"
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "${TARGET_RPATH}"
        )
    # No such thing as a CMake LINUX variable, contrary to APPLE and MSVC.
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    	set(TARGET_RPATH "$ORIGIN/../lib:$ORIGIN/../../USD/lib")
    	if(DEFINED PXR_USD_LOCATION)
    	    string(APPEND TARGET_RPATH ":${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64")
        endif()
		# Use RUNPATH instead of RPATH.
    	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags" PARENT_SCOPE)
        set_target_properties(${plugin}
                PROPERTIES
                INSTALL_RPATH "${TARGET_RPATH}"
                BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()
endfunction(set_translators_plugin_properties)

add_subdirectory(translators)
add_subdirectory(schemas)
