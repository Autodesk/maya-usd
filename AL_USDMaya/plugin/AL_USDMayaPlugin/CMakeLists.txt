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
else()
    set(rpath "$ORIGIN:$ORIGIN/../lib")
    if (CMAKE_INSTALL_RPATH)
        set(rpath "${CMAKE_INSTALL_RPATH}:${rpath}")
    endif()
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH "${rpath}"
    )
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION plugin
    RUNTIME
        DESTINATION plugin
)