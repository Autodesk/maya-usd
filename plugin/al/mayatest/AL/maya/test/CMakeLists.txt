
####################################################################################################
# Setup
####################################################################################################
find_package(GTest REQUIRED)

set(MAYA_TEST_LIBRARY_NAME "AL_MayaTest")
set(MAYA_TEST_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib)

####################################################################################################
# Source
####################################################################################################
add_library(${MAYA_TEST_LIBRARY_NAME} SHARED)

target_sources(${MAYA_TEST_LIBRARY_NAME}
  PRIVATE
    testHelpers.cpp
    testHarness.cpp
)

# compiler configuration
mayaUsd_compile_config(${MAYA_TEST_LIBRARY_NAME})

target_compile_definitions(${MAYA_TEST_LIBRARY_NAME}
  PRIVATE
    $<$<BOOL:${IS_MACOSX}>:OSMac_>
    $<$<BOOL:${IS_LINUX}>:LINUX>
    $<$<BOOL:${IS_WINDOWS}>:GTEST_LINKED_AS_SHARED_LIBRARY>
    AL_MAYA_TEST_EXPORT
)

target_link_libraries(${MAYA_TEST_LIBRARY_NAME}
  PUBLIC
    ${GTEST_LIBRARIES}
    ${MAYA_LIBRARIES}
    arch    # We should try and remove this dependency
    tf      # We should try and remove this dependency
)

target_include_directories(${MAYA_TEST_LIBRARY_NAME}
  PRIVATE
    ${MAYATEST_INCLUDE_LOCATION}
    ${GTEST_INCLUDE_DIRS}
    ${MAYA_INCLUDE_DIRS}
    ${USD_INCLUDE_DIR}
)

list(APPEND maya_test_headers
    Api.h
    testHelpers.h
    testHarness.h
)

# install the public headers
install( FILES ${maya_test_headers}
  DESTINATION ${AL_INSTALL_PREFIX}/include/AL/maya/test
)

# install the library
install(TARGETS ${MAYA_TEST_LIBRARY_NAME}
  LIBRARY
  DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
  RUNTIME
  DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
)
