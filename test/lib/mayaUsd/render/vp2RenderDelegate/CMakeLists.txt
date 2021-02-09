set(TARGET_NAME VP2_RENDER_DELEGATE_TEST)

# Unit test scripts.
set(TEST_SCRIPT_FILES "")

if (CMAKE_UFE_V2_FEATURES_AVAILABLE AND USD_VERSION_NUM GREATER_EQUAL 2005)
    # The point instance selection tests requires UsdImaging API version 13 or
    # later, which is only provided by core USD 20.05 or later.
    list(APPEND TEST_SCRIPT_FILES
        testVP2RenderDelegatePointInstanceSelection.py
        testVP2RenderDelegatePointInstancesPickMode.py
    )
endif()

add_custom_target(${TARGET_NAME} ALL)

# Copy all the resources and Python scripts to build directory
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    EXCLUDE "*.txt"
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "VP2_RENDER_DELEGATE_PROXY=1"

            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Fallback to old color management. We will have to investigate
            # and introduce OCIOv2 compatible version of these tests.
            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endforeach()
