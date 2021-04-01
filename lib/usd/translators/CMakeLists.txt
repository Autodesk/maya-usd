set(TARGET_NAME mayaUsd_Translators)

set(RESOURCES_INSTALL_PATH
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${TARGET_NAME}/resources
)

set(LIBRARY_INSTALL_PATH
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
        meshWriter_BlendShapes.cpp
        nurbsCurvesReader.cpp
        nurbsCurveWriter.cpp
        nurbsPatchReader.cpp
        nurbsSurfaceWriter.cpp
        particleWriter.cpp
        scopeReader.cpp
        skelReader.cpp
        strokeWriter.cpp
        xformReader.cpp
)

add_subdirectory(shading)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}
    PRIVATE
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
        basePxrUsdPreviewSurface
        ghc_filesystem
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
    DESTINATION ${RESOURCES_INSTALL_PATH}
)

install(
    TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${LIBRARY_INSTALL_PATH}
    ARCHIVE
    DESTINATION ${LIBRARY_INSTALL_PATH}
    RUNTIME
    DESTINATION ${LIBRARY_INSTALL_PATH}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}>
        DESTINATION ${LIBRARY_INSTALL_PATH} OPTIONAL
    )
endif()

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${TARGET_NAME}
)