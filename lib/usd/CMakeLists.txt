# promote headers
list(APPEND schemas_headers
    schemas/MayaReference.h
    schemas/api.h
    schemas/tokens.h
)

mayaUsd_promoteHeaderList(${schemas_headers})

if(BUILD_HDMAYA)
    add_subdirectory(hdMaya)
endif()
add_subdirectory(translators)
add_subdirectory(schemas)
