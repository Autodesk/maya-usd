find_package(GTest REQUIRED)

set(TARGET_NAME mayaUsdUtilsTests)
set(TEST_NAME GTest:mayaUsdUtilsTests)
set(PYTHON_TARGET_NAME Python:mayaUsdUtilsTests)

add_definitions(
  -DUSDUTILS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
)

add_executable(${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        test_MayaTransformAPI.cpp
        test_DiffCore.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PRIVATE 
        ${MAYAUSDUTILS_INCLUDE_LOCATION}
        ${USDUTILS_INCLUDE_LOCATION}
        ${GTEST_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE 
        GTest::GTest
        tf
        sdf
        ${boost_python_LIBRARIES}
        ${PYTHON_LIBRARIES}
        mayaUsdUtils
)

# -----------------------------------------------------------------------------
# unit tests
# -----------------------------------------------------------------------------
mayaUsd_add_test(${TARGET_NAME}
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
)

add_test(
  NAME ${PYTHON_TARGET_NAME}
  COMMAND
    python -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(${PYTHON_TARGET_NAME}
  PROPERTIES
    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/${INSTALL_DIR_SUFFIX}/lib/usd/utils:${CMAKE_BINARY_DIR}/src:$ENV{PYTHONPATH} 
)

if (TARGET all_tests)
  add_dependencies(all_tests ${TARGET_NAME} ${USDUTILS_PYTHON_LIBRARY_NAME})
endif()
