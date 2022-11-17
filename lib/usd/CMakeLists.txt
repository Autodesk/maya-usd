add_subdirectory(pxrUsdPreviewSurface)
add_subdirectory(translators)
add_subdirectory(schemas)

add_subdirectory(utils)

if(Qt5_FOUND)
    add_subdirectory(ui)
endif()

#install top level plugInfo.json that includes the configured plugInfo.json
set(INSTALL_DESTINATION 
    ${CMAKE_INSTALL_PREFIX}/lib/usd
)
install(CODE
    "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json\" \"{\n    \\\"Includes\\\": [ \\\"*/resources/\\\" ]\n}\")"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/usd/plugInfo.json
    DESTINATION ${INSTALL_DESTINATION}
)
