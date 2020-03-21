find_package(GTest REQUIRED)

set(USDTRANSACTION_TEST_EXECUTABLE_NAME AL_USDTransactionTests)
set(USDTRANSACTION_TEST_NAME GTest:AL_USDTransactionTests)
set(USDTRANSACTION_PYTHON_TEST_NAME Python:AL_USDTransactionTests)

add_executable(${USDTRANSACTION_TEST_EXECUTABLE_NAME})

target_sources(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    testMain.cpp
    testTransactionManager.cpp
    testTransaction.cpp
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

mayaUsd_add_test(${USDTRANSACTION_TEST_NAME}
    COMMAND ${USDTRANSACTION_TEST_EXECUTABLE_NAME}
)

mayaUsd_add_test(${USDTRANSACTION_PYTHON_TEST_NAME}
    COMMAND ${MAYA_PY_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)

if (TARGET all_tests)
  add_dependencies(all_tests ${USDTRANSACTION_TEST_EXECUTABLE_NAME} ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()