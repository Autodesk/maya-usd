set(TARGET_NAME mayaUsd_Translators)

set(install_destination 
    ${CMAKE_INSTALL_PREFIX}/lib/usd
)

set(resources_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources
)

set(library_install_path
    ${CMAKE_INSTALL_PREFIX}/lib
)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        cameraReader.cpp
        cameraWriter.cpp
        fileTextureWriter.cpp
        instancerWriter.cpp
        jointWriter.cpp
        lightReader.cpp
        lightWriter.cpp
        locatorWriter.cpp
        materialReader.cpp
        mayaReferenceReader.cpp
        mayaReferenceUpdater.cpp
        meshReader.cpp
        meshWriter.cpp
        meshWriter_Primvars.cpp
        meshWriter_Skin.cpp
        meshWriter_Subdiv.cpp
        nurbsCurveWriter.cpp
        nurbsCurvesReader.cpp
        nurbsPatchReader.cpp
        nurbsSurfaceWriter.cpp
        particleWriter.cpp
        scopeReader.cpp
        strokeWriter.cpp
        skelReader.cpp
        xformReader.cpp
)

# -----------------------------------------------------------------------------
# compile definitions
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME} 
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}
    PUBLIC
        ${MAYA_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME} 
    PRIVATE 
        arch
        gf
        kind
        sdf
        tf
        usd
        usdGeom
        usdLux
        usdRi
        usdShade
        usdSkel
        usdUtils
        vt
        ${MAYA_LIBRARIES}
        mayaUsd
        mayaUsd_Schemas
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json"
               "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${resources_install_path}
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
    DESTINATION ${install_destination}
)

install(
    TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${library_install_path}
    ARCHIVE
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
        DESTINATION ${library_install_path} OPTIONAL
    )
endif()
 