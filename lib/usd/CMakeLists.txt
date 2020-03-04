if(BUILD_HDMAYA)
    add_subdirectory(hdMaya)
endif()

add_subdirectory(translators)
add_subdirectory(schemas)

add_subdirectory(utils)

if(Qt5_FOUND)
    add_subdirectory(ui)
endif()
