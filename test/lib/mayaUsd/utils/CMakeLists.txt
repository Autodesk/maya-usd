# -----------------------------------------------------------------------------
# Python unit tests
# -----------------------------------------------------------------------------

set(TEST_SCRIPT_FILES
    testBlockSceneModificationContext.py
    testDiagnosticDelegate.py
)

if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND TEST_SCRIPT_FILES
        testUtilsEditability.py
    )
endif()

set(INTERACTIVE_TEST_SCRIPT_FILES "")
if(CMAKE_UFE_V2_FEATURES_AVAILABLE)
    list(APPEND INTERACTIVE_TEST_SCRIPT_FILES
        testUtilsSelectability.py
        testUtilsSelectabilityPointInstanceSelection.py
    )
endif()
        
foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS utils)
endforeach()

foreach(script ${INTERACTIVE_TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS utils)
endforeach()


# -----------------------------------------------------------------------------
# C++ unit tests
# -----------------------------------------------------------------------------

function(add_mayaUsdLibUtils_test TARGET_NAME)
    add_executable(${TARGET_NAME})

    # -----------------------------------------------------------------------------
    # sources
    # -----------------------------------------------------------------------------
    target_sources(${TARGET_NAME}
        PRIVATE
            main.cpp
            ${ARGN}
    )

    # -----------------------------------------------------------------------------
    # compiler configuration
    # -----------------------------------------------------------------------------
    mayaUsd_compile_config(${TARGET_NAME})

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    # -----------------------------------------------------------------------------
    # link libraries
    # -----------------------------------------------------------------------------
    target_link_libraries(${TARGET_NAME}
        PRIVATE 
            GTest::GTest
            mayaUsdUtils
            ${MAYA_LIBRARIES}
            mayaUsd
    )

    # handle run-time search paths
    if(IS_MACOSX OR IS_LINUX)
        mayaUsd_init_rpath(rpath $<TARGET_FILE_DIR:${TARGET_NAME}>)

        mayaUsd_add_rpath(rpath "../lib")
        if(BUILD_TESTS)
            mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
        endif()
        mayaUsd_add_rpath(rpath "${MAYA_LOCATION}/lib")
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")

        if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
            mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
        elseif(DEFINED PXR_USD_LOCATION)
            mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
        endif()
        if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
            mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
        endif()
        if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
            mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
        endif()
        mayaUsd_install_rpath(rpath ${TARGET_NAME})
    endif()

    # -----------------------------------------------------------------------------
    # unit tests
    # -----------------------------------------------------------------------------
    mayaUsd_add_test(${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LOCATION=${MAYA_LOCATION}"
    )
endfunction()

if(IS_MACOSX)
    # Create symbolic link to python framework
    file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${CMAKE_CURRENT_BINARY_DIR}/../Frameworks SYMBOLIC)
endif()

add_mayaUsdLibUtils_test(
    testLoadRules
    testLoadRules.cpp
)

