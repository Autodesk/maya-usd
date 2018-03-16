IF(DEFINED NO_TESTS AND NO_TESTS)
  message("Skipping test building.")
  return()
ENDIF()

enable_testing()

add_test(
    NAME TestAdditionalTranslators
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND testTranslators.sh ${CMAKE_INSTALL_PREFIX} $<TARGET_FILE_DIR:plug>
    )
