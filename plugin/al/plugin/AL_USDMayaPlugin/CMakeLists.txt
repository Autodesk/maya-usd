set(PXR_PACKAGE AL_USDMayaPlugin)

add_library(
    ${PXR_PACKAGE}
    plugin.cpp
)

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE
        AL_USDMAYA_PLUGIN_EXPORT
)

target_link_libraries(${PXR_PACKAGE}
    ar
    arch
    usdImaging
    usdImagingGL
    AL_EventSystem
    AL_USDMayaUtils
    AL_MayaUtils
    AL_USDUtils
    AL_USDMaya
    ${MAYA_LIBRARIES}
)

target_include_directories(${PXR_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
        "../../lib/AL_USDMaya"
)

if(MSVC)
    set_target_properties(${PXR_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            MACOSX_RPATH TRUE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "@loader_path/../lib;@loader_path/../../pxr/lib;@loader_path/../../pxr/maya/lib;${PXR_USD_LOCATION}/lib"
    )
else()
    # This puts the paths into RUNPATH instead of RPATH.
    # They are the same except that RPATH is searched before LD_LIBRARY_PATH whereas RUNPATH is searched after.
    # Using RUNPATH will give users the ability to override a shared library.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")

    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../pxr/lib:$ORIGIN/../../pxr/maya/lib:${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
    )
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PXR_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()
