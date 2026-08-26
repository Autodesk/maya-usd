# -----------------------------------------------------------------------------
# C++ unit tests
#
# MayaRendererProvider talks to a live Maya session (MEL "renderer" queries,
# defaultRenderGlobals) and to USD's Hydra plugin registry, so this test
# brings up Maya in library ("standalone") mode itself (see main.cpp) rather
# than relying on Python's maya.standalone, which the other C++ tests under
# test/lib/mayaUsd/utils don't need.
# -----------------------------------------------------------------------------
if(MAYA_HAS_USD_SETTINGS_NODES)
    add_executable(testMayaRendererProvider)

    target_sources(testMayaRendererProvider
        PRIVATE
        main.cpp
        testMayaRendererProvider.cpp
    )

    target_compile_definitions(testMayaRendererProvider
        PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
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

    if(IS_MACOSX)
        # Create symbolic link to python framework
        file(CREATE_LINK ${MAYA_LOCATION}/Frameworks ${CMAKE_CURRENT_BINARY_DIR}/../Frameworks SYMBOLIC)
    endif()

endif()
