find_package(GTest REQUIRED)

set(USDTRANSACTION_TEST_EXECUTABLE_NAME AL_USDTransactionTests)
set(USDTRANSACTION_TEST_NAME GTest:AL_USDTransactionTests)
set(USDTRANSACTION_PYTHON_TEST_NAME Python:AL_USDTransactionTests)

add_definitions(-DAL_EXTRAS_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data")

list(APPEND usdtransaction_test_source
    testMain.cpp
    testTransactionManager.cpp
    testTransaction.cpp
)

add_executable(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  ${usdtransaction_test_source}
)

target_link_libraries(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${GTEST_LIBRARIES}
    arch
    usd
    vt
    ${boost_python_LIBRARIES}
    ${USDTRANSACTION_LIBRARY_NAME}
    -lpthread -lm
)

target_include_directories(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${GTEST_INCLUDE_DIRS}
  PUBLIC 
    ${USDTRANSACTION_INCLUDE_LOCATION}
    ${USD_INCLUDE_DIR}
)

add_test(
  NAME ${USDTRANSACTION_TEST_NAME}
  COMMAND
    ${USDTRANSACTION_TEST_EXECUTABLE_NAME}
)

add_test(
  NAME ${USDTRANSACTION_PYTHON_TEST_NAME}
  COMMAND
    python -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(${USDTRANSACTION_PYTHON_TEST_NAME}
  PROPERTIES
    ENVIRONMENT PYTHONPATH=${CMAKE_BINARY_DIR}/${INSTALL_DIR_SUFFIX}/usdtransaction:${CMAKE_BINARY_DIR}/src:$ENV{PYTHONPATH} 
)


if (TARGET all_tests)
  add_dependencies(all_tests ${USDTRANSACTION_TEST_EXECUTABLE_NAME} ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()