project(hd-maya-al)

cmake_minimum_required(VERSION 2.8)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Maya REQUIRED)
find_package(MayaToHydra REQUIRED)

option(HD_MAYA_AL_OVERRIDE_PROXY_SELECTION "Override the AL_usdmaya_ProxyShape's selection to use Maya-to-Hydra's to find picked prims.  This is more efficient, as it doesn't require the ProxyShape to keep it's own GL copy of the scene, just ot use for selection, but requires a version of the AL plugin that has been modified to provide ProxyShape::setFindPickedPrimsFunction and getFindPickedPrimsFunction." OFF)


if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (LINUX)
    # Use RUNPATH, not RPATH - otherwise, LD_LIBRARY_PATH can't override the
    # built in path - this can create problems, for instance, if using a
    # different maya point release than the one you built against
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--enable-new-dtags")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--enable-new-dtags")

    if (NOT HDMAYA_LUMA_BUILD)
        # On Linux, USD will try to use <ext/hash_set> instead of <unordered_set>,
        # which will trigger a deprecation warning - so disable them. (In USD,
        # this is automatically done in gccclangshareddefaults, via:
        #     _disable_warning("deprecated")
        #     _disable_warning("deprecated-declarations")
        add_compile_options(-Wno-deprecated -Wno-deprecated-declarations)
    endif()
endif ()

add_subdirectory(src)
add_subdirectory(python)