
####################################################################################################
# Setup 
####################################################################################################

set(MAYA_TEST_LIBRARY_NAME "AL_MayaTest")
set(MAYA_TEST_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND maya_test_headers
    testHelpers.h
    testHarness.h
)

list(APPEND maya_test_source
    testHelpers.cpp
    testHarness.cpp
)

add_library(${MAYA_TEST_LIBRARY_NAME}
    SHARED
        ${maya_test_source}
)

target_compile_definitions(${MAYA_TEST_LIBRARY_NAME}
    PRIVATE
        AL_MAYA_TEST_EXPORT
)

target_link_libraries(${MAYA_TEST_LIBRARY_NAME}
    ${GTEST_LIBRARIES}
    ${MAYA_LIBRARIES}
)

# install the public headers
install( FILES ${maya_test_headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/maya/test
)

# install the library
install(TARGETS ${MAYA_TEST_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
)