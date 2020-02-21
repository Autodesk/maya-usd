set(TRANSLATORS_PLUGIN mayaUsd_Translators)

set(INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd)

set(
    resources_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/usd/${TRANSLATORS_PLUGIN}/resources
)

set(
    library_install_path
    ${CMAKE_INSTALL_PREFIX}/lib/
)

list(APPEND DEPENDANT_LIBRARIES 
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
    ${Boost_PYTHON_LIBRARY}
    ${MAYA_LIBRARIES}
    ${LIBRARY_NAME}
	mayaUsd_Schemas
)

list(APPEND source
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

add_library(${TRANSLATORS_PLUGIN} SHARED ${source})

#======================================================================
# TARGET PROPERTIES
#======================================================================
if(IS_MACOSX OR IS_LINUX)
    # rpath setup
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
    mayaUsd_install_rpath(rpath ${TRANSLATORS_PLUGIN})
endif()

if(IS_MACOSX)
    target_compile_definitions(${TRANSLATORS_PLUGIN} PRIVATE
        OSMac_
    )
endif()

#======================================================================
# INCLUDE, LINK
#======================================================================
target_include_directories(${TRANSLATORS_PLUGIN}
    PUBLIC
    ${MAYA_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(${TRANSLATORS_PLUGIN} PRIVATE ${DEPENDANT_LIBRARIES})

#======================================================================
# PLUGINFO
#======================================================================
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json"
  "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${resources_install_path}
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${INSTALL_DESTINATION}/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

#======================================================================
# INSTALL
#======================================================================

install(
    TARGETS ${TRANSLATORS_PLUGIN}
    LIBRARY
    DESTINATION ${library_install_path}
	ARCHIVE
    DESTINATION ${library_install_path}
    RUNTIME
    DESTINATION ${library_install_path}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TRANSLATORS_PLUGIN}> DESTINATION ${library_install_path} OPTIONAL)
endif()
