# -----------------------------------------------------------------------------
# C++ unit tests
#
# MayaRendererProvider and MayaRenderSetupHost talk to a live Maya session (MEL
# queries, proxy shapes, the undo queue) and to USD, so these tests bring up
# Maya in library ("standalone") mode themselves (see main.cpp) rather than
# relying on Python's maya.standalone, which the other C++ tests under
# test/lib/mayaUsd/utils don't need.
# -----------------------------------------------------------------------------
if(NOT IS_MACOSX)
    set(RENDER_SETUP_TESTS testMayaRenderSetupHost)
    if(MAYA_HAS_USD_SETTINGS_NODES)
        list(APPEND RENDER_SETUP_TESTS testMayaRendererProvider)
    endif()

    foreach(test_name ${RENDER_SETUP_TESTS})
        add_executable(${test_name})

        target_sources(${test_name}
            PRIVATE
            main.cpp
            ${test_name}.cpp
        )

        mayaUsd_compile_config(${test_name})

        # Must match mayaUsdUI, since the define changes the layout of the
        # classes under test.
        target_compile_definitions(${test_name}
            PRIVATE
            $<$<BOOL:${MAYA_HAS_USD_SETTINGS_NODES}>:MAYA_HAS_USD_SETTINGS_NODES>
        )

        target_include_directories(${test_name}
            PRIVATE
            ${ADSK_USD_RENDER_SETUP_INCLUDE_DIR}
        )

        target_link_libraries(${test_name}
            PRIVATE
            GTest::GTest
            ${MAYA_LIBRARIES}
            ${ADSK_USD_RENDER_SETUP_LIBRARY}
            mayaUsd
            mayaUsdUI
        )

        mayaUsd_add_test(${test_name}
            COMMAND $<TARGET_FILE:${test_name}>
            ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LOCATION=${MAYA_LOCATION}"
        )

        set_property(TEST ${test_name} APPEND PROPERTY LABELS SharedComponents)
    endforeach()
endif()
