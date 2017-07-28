IF(DEFINED NO_TESTS AND NO_TESTS)
  message("Skipping test building.")
  return()
ENDIF()

find_package(GTest REQUIRED)
add_subdirectory(test_plugin)

configure_file(
    "run_mayaplugin_tests.mel.in"
    "${CMAKE_CURRENT_BINARY_DIR}/run_mayaplugin_tests.mel"
)

configure_file(
    "run_mayaplugin_tests.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/run_mayaplugin_tests.sh"
)

add_custom_target(run_mayaplugin_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_mayaplugin_tests.sh
    DEPENDS AL_USDMayaTestPlugin ${SCHEMAS_PACKAGE} ${TRANSLATORS_PACKAGE}
)

if(MAYA_API_VERSION STRGREATER 2017 AND MAYA_API_VERSION STRLESS 2018)
    add_custom_target(maya_tests DEPENDS run_mayaplugin_tests)
    add_custom_target(unit_tests DEPENDS maya_tests)
else()
    message(WARNING "maya_tests not active since unsupported version of maya is available in the environment")
    add_custom_target(unit_tests)
endif()

add_custom_target(all_tests DEPENDS unit_tests)