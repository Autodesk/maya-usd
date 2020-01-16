set(TARGET_NAME testMayaSchemas)

find_package(GTest REQUIRED)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME} 
    PRIVATE
        main.cpp
        testMayaSchemas.cpp
        testModelAPI.cpp
)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../schemas
        ${GTEST_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
)

target_link_libraries(${TARGET_NAME}
    AL_USDMayaSchemas
    ${GTEST_LIBRARIES}
    sdf
    tf
    vt
    plug
    ${Boost_PYTHON_LIBRARY}
)

# install
install(TARGETS ${TARGET_NAME} DESTINATION ${AL_INSTALL_PREFIX}/bin)

# unit tests
set(path 
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib"
    "$ENV{PATH}"
)

if(IS_WINDOWS)
  list(APPEND path "${CMAKE_INSTALL_PREFIX}/lib/gtest")
endif()

set(pythonPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python"
    "$ENV{PYTHONPATH}"
)

set(pxr_pluginpath_name 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/usd"
    "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin"
)

set(mayaPluginPath 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/plugin"
)

if(IS_WINDOWS)
    string(REPLACE ";" "\;" pythonPath "${pythonPath}")
    string(REPLACE ";" "\;" path "${path}")
    string(REPLACE ";" "\;" pxr_pluginpath_name "${pxr_pluginpath_name}")
    string(REPLACE ";" "\;" mayaPluginPath "${mayaPluginPath}")
else()
    separate_arguments(pythonPath NATIVE_COMMAND "${pythonPath}")
    separate_arguments(path NATIVE_COMMAND "${path}")
    separate_arguments(pxr_pluginpath_name NATIVE_COMMAND "${pxr_pluginpath_name}")
    separate_arguments(mayaPluginPath NATIVE_COMMAND "${mayaPluginPath}")

    string(REPLACE "\;" ":" pythonPath "${pythonPath}")
    string(REPLACE "\;" ":" path "${path}")
    string(REPLACE "\;" ":" pxr_pluginpath_name "${pxr_pluginpath_name}")
    string(REPLACE "\;" ":" mayaPluginPath "${mayaPluginPath}")
endif()

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "bin")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "../../../lib/gtest")
    endif()
    mayaUsd_add_rpath(rpath "../lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

add_test(
    NAME ${TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
)

set_property(TEST ${TARGET_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "PXR_PLUGINPATH_NAME=${pxr_pluginpath_name}"
)
