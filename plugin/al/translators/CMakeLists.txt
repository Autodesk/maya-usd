# MayaReference provides import translation for AL_usd_MayaReference.
# As such, its AL_USDMAYA_DEFINE_TRANSLATOR does a
# TfType::Find<AL_usd_MayaReference>, which requires AL_usd_MayaReference's
# typeid.  We must therefore link against mayaUsd_Schemas.
list(APPEND DEPENDANT_LIBRARIES AL_USDMaya AL_USDMayaSchemas mayaUsd mayaUsd_Schemas gf plug tf)

set(DIRECTORY_PATH AL/usdmaya/fileio/translators)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in"
  "${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json"
  @ONLY
)

# usd plugin
add_library(${TRANSLATORS_PACKAGE}
    SHARED
    Camera.cpp
    CommonTranslatorOptions.cpp
    MayaReference.cpp
    Mesh.cpp
    NurbsCurve.cpp
    FrameRange.cpp
    DirectionalLight.cpp
)

# compiler configuration
mayaUsd_compile_config(${TRANSLATORS_PACKAGE})

# HS OCT 2019, do we really need to define
# AL_MAYA_MACROS_EXPORT for this target?
if(NOT IS_WINDOWS)
  target_compile_definitions(${TRANSLATORS_PACKAGE}
      PRIVATE
          AL_MAYA_MACROS_EXPORT
          $<$<BOOL:${IS_MACOSX}>:OSMac_>
          $<$<BOOL:${IS_LINUX}>:LINUX>
  )
endif()

set(arg_PUBLIC_HEADER_FILES
    Camera.h
)

# Copy the plugin metadata file in a hierarchy replicating the install hierarchy.
# The file will be used by the unittests.
get_target_property(LIBRARY_LOCATION
  ${TRANSLATORS_PACKAGE}
  LOCATION
  )

get_filename_component(LIBRARY_LOCATION
  ${LIBRARY_LOCATION}
  DIRECTORY
  )

configure_file(
  ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
  ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/${TRANSLATORS_PACKAGE}/resources/plugInfo.json
  )

# Setup the actual plugInfo.json
set(LIBRARY_LOCATION "../..")
configure_file(
  ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
  ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
  )

target_include_directories(
    ${TRANSLATORS_PACKAGE}
    PUBLIC
    ../schemas
    ../lib/AL_USDMaya
)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "../lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if (IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TRANSLATORS_PACKAGE})
endif()

target_link_libraries(${TRANSLATORS_PACKAGE} ${DEPENDANT_LIBRARIES})

install(TARGETS ${TRANSLATORS_PACKAGE}
 LIBRARY
 DESTINATION ${AL_INSTALL_PREFIX}/lib
 RUNTIME
 DESTINATION ${AL_INSTALL_PREFIX}/lib
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TRANSLATORS_PACKAGE}> DESTINATION ${AL_INSTALL_PREFIX}/lib OPTIONAL)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
 DESTINATION ${AL_INSTALL_PREFIX}/lib/usd/${TRANSLATORS_PACKAGE}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
 "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
 DESTINATION ${AL_INSTALL_PREFIX}/lib/usd
)

# install public headers
install(FILES ${arg_PUBLIC_HEADER_FILES}
     DESTINATION ${AL_INSTALL_PREFIX}/include/${DIRECTORY_PATH}
)

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()

option(BUILD_USDMAYA_PXR_TRANSLATORS
    "Build a translator for pxrUsd maya plugin, that allows it to export AL_usdmaya_ProxyShapes as references when usdExport is called."
    ON)
    
if(BUILD_USDMAYA_PXR_TRANSLATORS)
    set(PXR_TRANSLATORS_PACKAGE AL_USDMayaPxrTranslators)
    add_subdirectory(pxrUsdTranslators)
endif()
