list(APPEND DEPENDANT_LIBRARIES AL_USDMaya AL_USDMayaSchemas mayaUsd gf plug tf)

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

if(IS_MACOSX)
    set(_macDef OSMac_)
endif()

# HS OCT 2019, do we really need to define 
# AL_MAYA_MACROS_EXPORT for this target?
if(NOT IS_WINDOWS)
    set_target_properties(${TRANSLATORS_PACKAGE}
          PROPERTIES COMPILE_DEFINITIONS
          "AL_MAYA_MACROS_EXPORT;${_macDef}"
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

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TRANSLATORS_PACKAGE})
endif()

if (IS_LINUX)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags" PARENT_SCOPE)

  if(WANT_USD_RELATIVE_PATH)
      mayaUsd_add_rpath(rpath "../../../../USD/lib64")
  endif()
endif()

target_link_libraries(${TRANSLATORS_PACKAGE} ${DEPENDANT_LIBRARIES})

install(TARGETS ${TRANSLATORS_PACKAGE}
 LIBRARY
 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
 RUNTIME
 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TRANSLATORS_PACKAGE}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/${TRANSLATORS_PACKAGE}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
 "file(WRITE \"${CMAKE_INSTALL_PREFIX}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

# install public headers
install(FILES ${arg_PUBLIC_HEADER_FILES}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${DIRECTORY_PATH}
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
