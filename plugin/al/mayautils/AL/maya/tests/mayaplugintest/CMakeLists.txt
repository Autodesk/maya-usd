find_package(GTest REQUIRED)

set(MAYAUTILS_TEST_LIBRARY_NAME AL_MayaUtilsTests)

add_definitions(-DAL_EXTRAS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data")

list(APPEND mayaplugin_test_source
  utils/CommandGuiHelperTest.cpp
  utils/NodeHelperUnitTest.cpp
  utils/PluginTranslatorOptionsTest.cpp
  plugin.cpp
  test_maya_MenuBuilder.cpp
  test_maya_NodeHelper.cpp
  test_mobjectmap.cpp
)

add_library(${MAYAUTILS_TEST_LIBRARY_NAME}
    SHARED
      ${mayaplugin_test_source}
)

# Remove the lib prefix else Maya can't load the library
SET_TARGET_PROPERTIES(${MAYAUTILS_TEST_LIBRARY_NAME} PROPERTIES PREFIX "")

target_link_libraries(${MAYAUTILS_TEST_LIBRARY_NAME}
  PRIVATE
      ${GTEST_LIBRARIES}
      AL_MayaTest
      ${MAYAUTILS_LIBRARY_NAME}
)

target_include_directories(${MAYAUTILS_TEST_LIBRARY_NAME}
  PRIVATE
      ${GTEST_INCLUDE_DIRS}
      ${MAYATEST_INCLUDE_LOCATION}
)

set(MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/testplugins)

install(TARGETS ${MAYAUTILS_TEST_LIBRARY_NAME}
    RUNTIME
    DESTINATION ${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}
    LIBRARY
    DESTINATION ${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}
)

target_include_directories(${MAYAUTILS_TEST_LIBRARY_NAME}
    PUBLIC 
    ${MAYAUTILS_INCLUDE_LOCATION}
    )

add_test(
    NAME ${MAYAUTILS_TEST_LIBRARY_NAME}
    COMMAND
        ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
        ${MAYAUTILS_TEST_MAYAPLUGIN_INSTALL_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

