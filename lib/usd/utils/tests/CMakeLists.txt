find_package(GTest REQUIRED)

set(USDUTILS_TEST_EXECUTABLE_NAME AL_USDUtilsTests)
set(USDUTILS_TEST_NAME GTest:AL_USDUtilsTests)
set(USDUTILS_PYTHON_TEST_NAME Python:AL_USDUtilsTests)

add_definitions(
  -DAL_EXTRAS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

list(APPEND usdutils_test_source
    main.cpp
    test_MayaTransformAPI.cpp
)

add_executable(${USDUTILS_TEST_EXECUTABLE_NAME}
  ${usdutils_test_source}
)

target_link_libraries(${USDUTILS_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${boost_python_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${PYTHON_LIBRARIES}
    arch
    sdf
    tf
    gf
    usd
    usdGeom
    vt
    mayaUsdUtils
    -lpthread -lm
)

target_include_directories(${USDUTILS_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${GTEST_INCLUDE_DIRS}
  PUBLIC 
    ${USDUTILS_INCLUDE_LOCATION}
    ${USD_INCLUDE_DIR}
)

add_test(
  NAME ${USDUTILS_TEST_NAME}
  COMMAND
    ${USDUTILS_TEST_EXECUTABLE_NAME}
)

add_test(
  NAME ${USDUTILS_PYTHON_TEST_NAME}
  COMMAND
    python -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(${USDUTILS_PYTHON_TEST_NAME}
  PROPERTIES
    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/${INSTALL_DIR_SUFFIX}/usdutils:${CMAKE_BINARY_DIR}/src:$ENV{PYTHONPATH} 
)


if (TARGET all_tests)
  add_dependencies(all_tests ${USDUTILS_TEST_EXECUTABLE_NAME} ${USDUTILS_PYTHON_LIBRARY_NAME})
endif()