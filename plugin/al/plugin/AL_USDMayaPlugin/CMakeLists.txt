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

if(IS_WINDOWS)
    set_target_properties(${PXR_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(IS_MACOSX)
    set_target_properties(${PXR_PACKAGE} PROPERTIES PREFIX "" SUFFIX ".bundle")
elseif(IS_LINUX)
    set_target_properties(${PXR_PACKAGE} PROPERTIES PREFIX "")
endif()

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    init_rpath(rpath "plugin")
    add_rpath(rpath "../lib")
    if(WANT_USD_RELATIVE_PATH)
        add_rpath(rpath "../../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    install_rpath(rpath ${PXR_PACKAGE})
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PXR_PACKAGE}> DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin OPTIONAL)
endif()
