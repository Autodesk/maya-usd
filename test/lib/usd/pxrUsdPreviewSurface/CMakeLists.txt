set(TARGET_NAME MAYAUSD_PXR_USD_PREVIEW_SURFACE_TEST)

# Unit test scripts.
set(TEST_SCRIPT_FILES
    testPxrUsdPreviewSurfaceDraw.py
)

if(BUILD_PXR_PLUGIN)
    # This test uses the file "PxrUsdPreviewSurfaceExportTest.ma" which
    # requires the plugin "pxrUsdPreviewSurface" that is built by the
    # Pixar plugin.
    list(APPEND TEST_SCRIPT_FILES
        testPxrUsdPreviewSurfaceExport.py
    )
endif()

# copy tests to ${CMAKE_CURRENT_BINARY_DIR} and run them from there
add_custom_target(${TARGET_NAME} ALL)

# copy test files to build directory
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    EXCLUDE "CMakeLists.txt"
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_MODULE ${target}
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS usdPreviewSurface)
endforeach()
