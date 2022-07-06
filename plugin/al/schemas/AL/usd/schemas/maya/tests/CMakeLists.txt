set(TARGET_NAME testMayaSchemas)

find_package(GTest REQUIRED)

add_executable(${TARGET_NAME})

# compiler configuration
mayaUsd_compile_config(${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        testMayaSchemas.cpp
        testModelAPI.cpp
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/schemas
        ${GTEST_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
)

target_link_libraries(${TARGET_NAME}
    AL_USDMayaSchemas
    mayaUsd_Schemas
    ${GTEST_LIBRARIES}
    sdf
    tf
    vt
    plug
)

# install
install(TARGETS ${TARGET_NAME} DESTINATION ${AL_INSTALL_PREFIX}/bin)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "bin")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    mayaUsd_add_rpath(rpath "../lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if(IS_MACOSX)
    # Create symbolic link to python framework
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${CMAKE_CURRENT_BINARY_DIR}/../Frameworks SYMBOLIC)
    file(MAKE_DIRECTORY ${AL_INSTALL_PREFIX})
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${AL_INSTALL_PREFIX}/Frameworks SYMBOLIC)
endif()

# unit tests

mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
