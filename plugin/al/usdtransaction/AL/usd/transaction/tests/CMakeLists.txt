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

add_test(
  NAME ${USDTRANSACTION_TEST_NAME}
  COMMAND
    ${USDTRANSACTION_TEST_EXECUTABLE_NAME}
)

if(IS_WINDOWS)
  set(path
      "${CMAKE_INSTALL_PREFIX}/lib/gtest"
      "${CMAKE_INSTALL_PREFIX}/plugin/al/lib"
      "$ENV{PATH}"
  )
  string(REPLACE ";" "\;" path "${path}")

  set_property(TEST ${USDTRANSACTION_TEST_NAME} APPEND PROPERTY ENVIRONMENT 
      "PATH=${path}"
  )
endif()

add_test(
  NAME ${USDTRANSACTION_PYTHON_TEST_NAME}
  COMMAND
    ${MAYA_PY_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
)

# set the PYTHONPATH and PATH for the python unittests
set(pythonPath
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib/python"
    "$ENV{PYTHONPATH}"
)

set(path 
    "${CMAKE_INSTALL_PREFIX}/plugin/al/lib"
    "$ENV{PATH}"
)

if(IS_WINDOWS)
  list(APPEND path "${CMAKE_INSTALL_PREFIX}/lib/gtest")
endif()

separate_argument_list(path)
separate_argument_list(pythonPath)

set_property(TEST ${USDTRANSACTION_PYTHON_TEST_NAME} APPEND PROPERTY ENVIRONMENT 
    "PYTHONPATH=${pythonPath}"
    "PATH=${path}"
)

if (TARGET all_tests)
  add_dependencies(all_tests ${USDTRANSACTION_TEST_EXECUTABLE_NAME} ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()