list(APPEND DEPENDANT_LIBRARIES AL_USDMaya ${SCHEMAS_PACKAGE} gf plug tf)

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
    MayaReference.cpp
    Mesh.cpp
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
  ${CMAKE_CURRENT_BINARY_DIR}/share/usd/plugins/${TRANSLATORS_PACKAGE}/resources/plugInfo.json
  )

# Setup the actual plugInfo.json
set(LIBRARY_LOCATION "../../../../lib")
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

set_target_properties(${TRANSLATORS_PACKAGE}
 PROPERTIES COMPILE_DEFINITIONS
     "MFB_TRANSLATORS_PACKAGE=${TRANSLATORS_PACKAGE};MFB_ALT_TRANSLATORS_PACKAGE=${TRANSLATORS_PACKAGE}"
)

target_link_libraries(${TRANSLATORS_PACKAGE} ${DEPENDANT_LIBRARIES})

install(TARGETS ${TRANSLATORS_PACKAGE}
 LIBRARY
 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/usd/plugins/${TRANSLATORS_PACKAGE}/resources
)

#install top level plugInfo.json that includes the configured plugInfo.json
install(CODE
 "file(WRITE \"${CMAKE_INSTALL_PREFIX}/share/usd/plugins/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)

# install public headers
install(FILES ${arg_PUBLIC_HEADER_FILES}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${DIRECTORY_PATH}
)

add_subdirectory(tests)