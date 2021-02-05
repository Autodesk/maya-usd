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

# compiler configuration
mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
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

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "testplugins")

    mayaUsd_add_rpath(rpath "../lib")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    mayaUsd_add_rpath(rpath "${MAYA_LOCATION}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")

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

# unit tests

set(pycode "
import sys
import maya.cmds
maya.cmds.loadPlugin('AL_MayaUtilsTests')
sys.exit(maya.cmds.AL_maya_test_UnitTestHarness())
")

mayaUsd_add_test(${TARGET_NAME}
    PYTHON_COMMAND "${pycode}"
    ENV
        "MAYA_PLUG_IN_PATH=${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}"
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
