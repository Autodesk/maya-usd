set(TARGET_NAME AL_USDTransactionTests)

find_package(GTest REQUIRED)

add_executable(${TARGET_NAME})

# compiler configuration
mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
)

target_sources(${TARGET_NAME}
  PRIVATE
    testMain.cpp
    testTransactionManager.cpp
    testTransaction.cpp
)

target_include_directories(${TARGET_NAME}
  PUBLIC
    ${GTEST_INCLUDE_DIRS}
    ${USDTRANSACTION_INCLUDE_LOCATION}
    ${PXR_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME}
    ${GTEST_LIBRARIES}
    arch
    usd
    vt
    $<$<BOOL:${IS_LINUX}>:${Boost_PYTHON_LIBRARY}>
    ${USDTRANSACTION_LIBRARY_NAME}
)

# install
install(TARGETS ${TARGET_NAME} DESTINATION ${AL_INSTALL_PREFIX}/bin)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "bin")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    mayaUsd_add_rpath(rpath "../lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if(IS_MACOSX)
    # Create symbolic link to python framework
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${CMAKE_CURRENT_BINARY_DIR}/../Frameworks SYMBOLIC)
    file(MAKE_DIRECTORY ${AL_INSTALL_PREFIX})
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${AL_INSTALL_PREFIX}/Frameworks SYMBOLIC)
endif()

# Disable the test for now here to simplify tests: MAYA-124144 MayaUsd: Maya OSX blessing failing GTest:AL_USDTransactionTests
if (NOT MAYA_APP_VERSION VERSION_GREATER 2023 OR NOT IS_MACOSX)
    mayaUsd_add_test(GTest:${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "DYLD_PRINT_LIBRARIES=YES"
    )
endif()

mayaUsd_add_test(Python:${TARGET_NAME}
    COMMAND ${MAYA_PY_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}
    ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

if (TARGET all_tests)
  add_dependencies(all_tests ${TARGET_NAME} ${USDTRANSACTION_PYTHON_LIBRARY_NAME})
endif()