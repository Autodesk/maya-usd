set(TRANSLATORS_PLUGIN mayaUsdTranslators)

set(INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin)

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
    mayaUsd
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
    skelReader.cpp
    xformReader.cpp
)

add_library(${TRANSLATORS_PLUGIN} SHARED ${source})

#======================================================================
# TARGET PROPERTIES
#======================================================================
if(MSVC)
    set_target_properties(${TRANSLATORS_PLUGIN} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set_target_properties(${TRANSLATORS_PLUGIN}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@loader_path/../lib;@loader_path/../../USD/lib;@loader_path/../../USD/third_party/maya/lib;${PXR_USD_LOCATION}/lib"
    )
else()
    set_target_properties(${TRANSLATORS_PLUGIN}
            PROPERTIES
            PREFIX ""
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../USD/lib:$ORIGIN/../../USD/third_party/maya/lib:${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
            BUILD_WITH_INSTALL_RPATH TRUE
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

target_include_directories(${TRANSLATORS_PLUGIN}
    PRIVATE
    api.h
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
    DESTINATION ${INSTALL_DESTINATION}/${TRANSLATORS_PLUGIN}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
    "file(WRITE \"${INSTALL_DESTINATION}/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

#======================================================================
# INSTALL
#======================================================================
# TODO header files? 

install(TARGETS ${TRANSLATORS_PLUGIN}
    LIBRARY
    DESTINATION ${INSTALL_DESTINATION}
    RUNTIME
    DESTINATION ${INSTALL_DESTINATION}
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${TRANSLATORS_PLUGIN}> DESTINATION ${INSTALL_DESTINATION} OPTIONAL)
endif()
