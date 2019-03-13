# In case we need boost, USD still needs the dynamic ones.
# This is the default value.
# set(Boost_USE_STATIC_LIBS OFF)

find_package(Maya)
find_package(OpenGL REQUIRED)
find_package(USD 0.18.11 REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)

link_directories(${USD_LIBRARY_DIR})
link_directories(${USD_MAYA_LIBRARY_DIR})

set (USD_001901_BUILD OFF)
set (USD_001903_BUILD OFF)
set (USD_001905_BUILD OFF)
if (USD_VERSION VERSION_GREATER "0.18.11")
    set (USD_001901_BUILD ON)
    if (USD_VERSION VERSION_GREATER "0.19.01")
        set (USD_001903_BUILD ON)
        if (USD_VERSION VERSION_GREATER "0.19.03")
            set (USD_001905_BUILD ON)
        endif ()
    endif ()
endif ()

if(HDMAYA_UFE_BUILD)
    # make sure HDMAYA_UFE_BUILD_INT is an int, for use in #define / configure
    set(HDMAYA_UFE_BUILD_INT 1)

    message(STATUS "UFE Build Enabled")
    find_package(UFE REQUIRED)
    message(STATUS "Using UFE version : ${UFE_VERSION}")
    message(STATUS "UFE Include directory: ${UFE_INCLUDE_DIR}")
    message(STATUS "UFE Library directory: ${UFE_LIBRARY_DIR}")
    include_directories(${UFE_INCLUDE_DIR})
else()
    # make sure HDMAYA_UFE_BUILD is an int, for use in #define / configure
    set(HDMAYA_UFE_BUILD_INT 0)
endif()

add_definitions(-DHDMAYA_UFE_BUILD=${HDMAYA_UFE_BUILD_INT})


set(MAYA_LIBS
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaUI_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY})

add_subdirectory(hdmaya)
add_subdirectory(plugin)
