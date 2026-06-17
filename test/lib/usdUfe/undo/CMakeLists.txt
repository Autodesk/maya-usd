function(add_mayaUsdUfeUndo_test TARGET_NAME)
    add_executable(${TARGET_NAME})

    target_sources(${TARGET_NAME}
        PRIVATE
            main.cpp
            ${ARGN}
    )

    mayaUsd_compile_config(${TARGET_NAME})

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    target_link_libraries(${TARGET_NAME}
        PRIVATE
            GTest::GTest
            usdUfe
            usdRender
    )

    mayaUsd_add_test(${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endfunction()

add_mayaUsdUfeUndo_test(
    testUsdUfeUndo
    test_UsdUndoUtils.cpp
)
