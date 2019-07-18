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
        "../../lib/AL_USDMaya"
)

if(MSVC)
    set_target_properties(${PXR_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../lib;@loader_path/../../USD/lib;@loader_path/../../USD/third_party/maya/lib;${PXR_USD_LOCATION}/lib"
    )
else()
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../USD/lib:$ORIGIN/../../USD/third_party/maya/lib:${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
    )
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PXR_PACKAGE}> DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin OPTIONAL)
endif()
