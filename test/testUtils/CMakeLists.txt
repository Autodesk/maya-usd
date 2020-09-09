set(TARGET_NAME PYTHON_TEST_UTILS)

add_custom_target(${TARGET_NAME} ALL)

# Copy all of the resources and Python scripts in this directory to a
# "test/python" subdirectory under the top-level build directory. This path
# will be added to PYTHONPATH for tests so that these utilities are available
# to all tests.
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_BINARY_DIR}/test/python
    EXCLUDE "*.txt"
)
