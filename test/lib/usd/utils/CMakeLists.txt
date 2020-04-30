set(TARGET_NAME DiffCore)

add_executable(${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        test_DiffCore.cpp
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE 
        GTest::GTest
        mayaUsdUtils
)

# -----------------------------------------------------------------------------
# environment variables
# -----------------------------------------------------------------------------
add_definitions(
  -DAL_EXTRAS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# -----------------------------------------------------------------------------
# unit tests
# -----------------------------------------------------------------------------
mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
)

set(TARGET_NAME MayaTransformAPI)

add_executable(${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        test_MayaTransformAPI.cpp
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE 
        GTest::GTest
        mayaUsdUtils
        tf
        sdf
        ${Boost_PYTHON_LIBRARY}
        ${python_LIBRARIES}
)

# -----------------------------------------------------------------------------
# environment variables
# -----------------------------------------------------------------------------
add_definitions(
  -DUSDUTILS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
)

mayaUsd_copyDirectory(${TARGET_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                             DIRECTORY data)
# -----------------------------------------------------------------------------
# unit tests
# -----------------------------------------------------------------------------

mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
)

# -----------------------------------------------------------------------------
# python tests - using older approach as mayaUsd_add_test wants to use mayapy
# -----------------------------------------------------------------------------

set(USDUTILS_PYTHON_TEST_NAME Python:MayaTransformAPI)
add_test(
  NAME ${USDUTILS_PYTHON_TEST_NAME}
  COMMAND
    python -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(${USDUTILS_PYTHON_TEST_NAME}
  PROPERTIES
    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/${INSTALL_DIR_SUFFIX}/usdutils:${CMAKE_BINARY_DIR}/${INSTALL_DIR_SUFFIX}/lib/usd/utils:$ENV{PYTHONPATH} 
)