set(TARGET_NAME PYTHON_TEST_SAMPLES)

add_custom_target(${TARGET_NAME} ALL)

# Copy all of the resources in this directory to a
# "test/testSamples" subdirectory under the top-level build directory.
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_BINARY_DIR}/test/testSamples
    EXCLUDE "*.txt"
)
