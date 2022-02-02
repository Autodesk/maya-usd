add_subdirectory(AL/usd/transaction)
set(LIBRARY_NAME AL_USDTransaction)

set(PY_INIT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/AL/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/AL/usd/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/AL/usd/transaction/__init__.py
)

# copy to build location for module tests
foreach(INPUT_FILE ${PY_INIT_FILES})
  string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_FILE ${INPUT_FILE})
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${INPUT_FILE}" "${OUTPUT_FILE}")
endforeach()

set(libFile ${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
set(PLUG_INFO_RESOURCE_PATH "resources")
set(PLUG_INFO_ROOT "..")
set(PLUG_INFO_PLUGIN_NAME "pxr.${LIBRARY_NAME}")
set(PLUG_INFO_LIBRARY_PATH "../../${libFile}")
configure_file(
    plugInfo.json
    ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
)

####################################################################################################
# Install 
####################################################################################################

# install python module
foreach(INPUT_FILE ${PY_INIT_FILES})
  string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${AL_INSTALL_PREFIX}/lib/python OUTPUT_FILE ${INPUT_FILE})
  get_filename_component(OUTPUT_PATH ${OUTPUT_FILE} DIRECTORY)
  install(FILES
        ${INPUT_FILE}  # .py files
      DESTINATION ${OUTPUT_PATH}
  )
endforeach()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json
    DESTINATION ${AL_INSTALL_PREFIX}/lib/usd/${LIBRARY_NAME}/resources
)
install(CODE
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
    DESTINATION ${AL_INSTALL_PREFIX}/lib/usd
)