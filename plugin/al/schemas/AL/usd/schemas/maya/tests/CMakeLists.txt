IF(SKIP_USDMAYA_TESTS)
  message("Skipping test building.")
  return()
ENDIF()

find_package(GTest REQUIRED)

add_executable(testMayaSchemas
    main.cpp
    testMayaSchemas.cpp
    testModelAPI.cpp
    )

target_include_directories(testMayaSchemas
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../schemas
    ${GTEST_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR}
    )

target_link_libraries(testMayaSchemas
    AL_USDMayaSchemas
    ${GTEST_LIBRARIES}
    sdf
    tf
    vt
    plug
    ${Boost_PYTHON_LIBRARY}
    )

install(TARGETS testMayaSchemas DESTINATION ${INSTALL_DIR_SUFFIX}/bin)


add_test(
    NAME TestAdditionalSchemas
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND RunTests.sh ${CMAKE_INSTALL_PREFIX} ${USD_LIBRARY_DIR}
)
