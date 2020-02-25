if(BUILD_HDMAYA)
    add_subdirectory(hdMaya)
endif()

add_subdirectory(translators)
add_subdirectory(schemas)

if(Qt5_FOUND)
    add_subdirectory(ui)
endif()
