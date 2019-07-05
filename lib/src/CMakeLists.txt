set(LIBRARY_NAME mayaUsd)

list(APPEND src
    core.cpp
)

list(APPEND header_install
    ../include/api.h
)

add_library(${LIBRARY_NAME}
    SHARED
        ${src}
)

target_compile_definitions(${LIBRARY_NAME}
    PRIVATE
        MAYAUSD_MACROS_EXPORT
        MAYAUSD_CORE_EXPORT
        MFB_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_ALT_PACKAGE_NAME="${LIBRARY_NAME}"
        MFB_PACKAGE_MODULE=mayaUsd
)

target_include_directories(
    ${LIBRARY_NAME}
    PUBLIC
    ${MAYA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    # mayaUsd.h is generated in the build area
    ${CMAKE_CURRENT_BINARY_DIR}/../include
)

target_link_libraries(${LIBRARY_NAME}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMaya_LIBRARY}
)

install(TARGETS ${LIBRARY_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

set_property(GLOBAL PROPERTY GLOBAL_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# install public headers
install(FILES ${header_install}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../include/mayaUsd.h
     DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd
)
