set(TARGET_NAME DiffCore)

add_executable(${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        test_DiffCore.cpp
        test_DiffValues.cpp
        test_DiffValuesArrays.cpp
        test_DiffValuesGeneric.cpp
        test_DiffValuesMatrices.cpp
        test_DiffValuesDoubleVecs.cpp
        test_DiffValuesFloatVecs.cpp
        test_DiffValuesHalfVecs.cpp
        test_DiffValuesIntVecs.cpp
        test_DiffAttributes.cpp
        test_DiffPrims.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
mayaUsd_compile_config(${TARGET_NAME})

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
