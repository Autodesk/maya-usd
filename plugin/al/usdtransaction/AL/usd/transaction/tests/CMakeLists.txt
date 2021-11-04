find_package(GTest REQUIRED)

set(USDTRANSACTION_TEST_EXECUTABLE_NAME AL_USDTransactionTests)
set(USDTRANSACTION_TEST_NAME GTest:AL_USDTransactionTests)
set(USDTRANSACTION_PYTHON_TEST_NAME Python:AL_USDTransactionTests)

add_executable(${USDTRANSACTION_TEST_EXECUTABLE_NAME})

# compiler configuration
mayaUsd_compile_config(${USDTRANSACTION_TEST_EXECUTABLE_NAME})

target_compile_definitions(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
    PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

target_sources(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    testMain.cpp
    testTransactionManager.cpp
    testTransaction.cpp
)

if(IS_LINUX)
    set(PTHREAD_LINK -lpthread -lm)
endif()

target_link_libraries(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${GTEST_LIBRARIES}
    arch
    usd
    vt
    ${boost_python_LIBRARIES}
    ${USDTRANSACTION_LIBRARY_NAME}
    ${PTHREAD_LINK}
)

if(IS_MACOSX)
    # Create symbolic link to python framework
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${CMAKE_CURRENT_BINARY_DIR}/../Frameworks SYMBOLIC)
endif()

target_include_directories(${USDTRANSACTION_TEST_EXECUTABLE_NAME}
  PRIVATE
    ${GTEST_INCLUDE_DIRS}
  PUBLIC
    ${USDTRANSACTION_INCLUDE_LOCATION}
    ${USD_INCLUDE_DIR}
)

mayaUsd_add_test(${USDTRANSACTION_TEST_NAME}
    COMMAND ${USDTRANSACTION_TEST_EXECUTABLE_NAME}
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

mayaUsd_add_test(${USDTRANSACTION_PYTHON_TEST_NAME}
    COMMAND ${MAYA_PY_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

if (TARGET all_tests)
  add_dependencies(all_tests ${USDTRANSACTION_TEST_EXECUTABLE_NAME} ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()