# -----------------------------------------------------------------------------
# C++ unit tests
#
# MayaRendererProvider talks to a live Maya session (MEL "renderer" queries,
# defaultRenderGlobals) and to USD's Hydra plugin registry, so this test
# brings up Maya in library ("standalone") mode itself (see main.cpp) rather
# than relying on Python's maya.standalone, which the other C++ tests under
# test/lib/mayaUsd/utils don't need.
# -----------------------------------------------------------------------------
if(MAYA_HAS_USD_SETTINGS_NODES AND NOT IS_MACOSX)
    add_executable(testMayaRendererProvider)

    target_sources(testMayaRendererProvider
        PRIVATE
        main.cpp
        testMayaRendererProvider.cpp
    )

    mayaUsd_compile_config(testMayaRendererProvider)

    target_include_directories(testMayaRendererProvider
        PRIVATE
        ${ADSK_USD_RENDER_SETUP_INCLUDE_DIR}
    )

    target_link_libraries(testMayaRendererProvider
        PRIVATE
        GTest::GTest
        ${MAYA_LIBRARIES}
        mayaUsd
        mayaUsdUI
    )

    mayaUsd_add_test(testMayaRendererProvider
        COMMAND $<TARGET_FILE:testMayaRendererProvider>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )

    set_property(TEST testMayaRendererProvider APPEND PROPERTY LABELS SharedComponents)
endif()
