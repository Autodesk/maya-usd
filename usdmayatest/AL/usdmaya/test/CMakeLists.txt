
####################################################################################################
# Setup 
####################################################################################################

set(USDMAYA_TEST_LIBRARY_NAME "AL_USDMayaTest")
set(MAYA_TEST_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND usdmaya_test_headers
    testHelpers.h
    testHarness.h
)

list(APPEND usdmaya_test_source
    testHelpers.cpp
    testHarness.cpp
)

add_library(${USDMAYA_TEST_LIBRARY_NAME}
    SHARED
        ${usdmaya_test_source}
)

target_compile_definitions(${USDMAYA_TEST_LIBRARY_NAME}
    PRIVATE
        AL_USDMAYA_TEST_EXPORT
)

target_link_libraries(${USDMAYA_TEST_LIBRARY_NAME}
    AL_EventSystem
    AL_USDMayaUtils
    AL_MayaUtils
    AL_USDUtils
    AL_USDMaya
    ${GTEST_LIBRARIES}
    ${MAYA_LIBRARIES}
)

target_include_directories(${USDMAYA_TEST_LIBRARY_NAME} 
    PUBLIC
    ${USDMAYATEST_INCLUDE_LOCATION}
)

# install the public headers
install( FILES ${usdmaya_test_headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/usdmaya/test
)

# install the library
install(TARGETS ${USDMAYA_TEST_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${MAYA_TEST_LIBRARY_LOCATION}
)