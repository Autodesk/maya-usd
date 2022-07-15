function(add_mayaUsdUtils_test TARGET_NAME)
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
    )

    # -----------------------------------------------------------------------------
    # unit tests
    # -----------------------------------------------------------------------------
    mayaUsd_add_test(${TARGET_NAME}
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )
endfunction()

add_mayaUsdUtils_test(
    testDiffCore
    test_DiffCore.cpp
)

add_mayaUsdUtils_test(
    testDiffValues
    test_DiffValues.cpp
    test_DiffValuesArrays.cpp
    test_DiffValuesGeneric.cpp
    test_DiffValuesMatrices.cpp
    test_DiffValuesQuaternions.cpp
    test_DiffValuesDoubleVecs.cpp
    test_DiffValuesFloatVecs.cpp
    test_DiffValuesHalfVecs.cpp
    test_DiffValuesIntVecs.cpp
)

add_mayaUsdUtils_test(
    testDiffAttributes
    test_DiffAttributes.cpp
)

add_mayaUsdUtils_test(
    testDiffRelationships
    test_DiffRelationships.cpp
)

add_mayaUsdUtils_test(
    testDiffDictionaries
    test_DiffDictionaries.cpp
)

add_mayaUsdUtils_test(
    testDiffLists
    test_DiffLists.cpp
)

add_mayaUsdUtils_test(
    testDiffComputeOverall
    test_DiffComputeOverall.cpp
)

add_mayaUsdUtils_test(
    testDiffPrims
    test_DiffPrimsAttributes.cpp
    test_DiffPrimsRelations.cpp
    test_DiffPrimsChildren.cpp
    test_DiffPrims.cpp
)

add_mayaUsdUtils_test(
    testMergePrims
    test_MergePrims.cpp
)

add_mayaUsdUtils_test(
    testDiffMetadatas
    test_DiffMetadatas.cpp
)

