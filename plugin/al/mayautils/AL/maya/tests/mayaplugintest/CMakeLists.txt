set(TARGET_NAME AL_MayaUtilsTests)

find_package(GTest REQUIRED)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME} 
    PRIVATE
        plugin.cpp
        test_maya_MenuBuilder.cpp
        test_maya_NodeHelper.cpp
        test_mobjectmap.cpp
        utils/CommandGuiHelperTest.cpp
        utils/NodeHelperUnitTest.cpp
        utils/PluginTranslatorOptionsTest.cpp
)

if(IS_WINDOWS)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".mll")
elseif(IS_MACOSX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".bundle")
elseif(IS_LINUX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
endif()

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

target_link_libraries(${TARGET_NAME}
  PRIVATE
      ${GTEST_LIBRARIES}
      AL_MayaTest
      ${MAYAUTILS_LIBRARY_NAME}
)

target_include_directories(${TARGET_NAME}
  PRIVATE
      ${GTEST_INCLUDE_DIRS}
      ${MAYATEST_INCLUDE_LOCATION}
)

set(MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH ${AL_INSTALL_PREFIX}/testplugins)

install(TARGETS ${TARGET_NAME}
    RUNTIME
    DESTINATION ${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}
    LIBRARY
    DESTINATION ${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}
)

target_include_directories(${TARGET_NAME}
    PUBLIC 
    ${MAYAUTILS_INCLUDE_LOCATION}
)

# unit tests
set(path 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib" # AL_MayaTest, AL_MayaUtils
    "$ENV{PATH}"
)

if(IS_WINDOWS)
  list(APPEND path "${CMAKE_INSTALL_PREFIX}/lib/gtest")
endif()

set(pythonPath 
    "$ENV{PYTHONPATH}"
)

set(mayaPluginPath 
    "${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}"
)

separate_argument_list(path)
separate_argument_list(pythonPath)
separate_argument_list(mayaPluginPath)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "testplugins")

    mayaUsd_add_rpath(rpath "../lib")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    mayaUsd_add_rpath(rpath "${MAYA_LOCATION}/lib")

    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

add_test(
    NAME ${TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${MAYA_PY_EXECUTABLE} -c "import sys;import maya.standalone; \
                                      maya.standalone.initialize(name='python'); \
                                      maya.cmds.loadPlugin('AL_MayaUtilsTests'); \
                                      ret=maya.mel.eval('AL_maya_test_UnitTestHarness'); \
                                      maya.standalone.uninitialize(); \
                                      "
)

set_property(TEST ${TARGET_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
    "MAYA_PLUG_IN_PATH=${mayaPluginPath}"
    "MAYA_NO_STANDALONE_ATEXIT=1"
)