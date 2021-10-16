set(TARGET_NAME MAYAUSD_NODES_TEST)

set(TEST_SCRIPT_FILES
    testLayerManagerSerialization.py
    testPointBasedDeformerNode.py
    testProxyShapeBase.py
    testProxyShapeMayaReference.py
)

add_custom_target(${TARGET_NAME} ALL)

# copy all the resources and Python scripts to build directory
mayaUsd_copyDirectory(${TARGET_NAME}
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    EXCLUDE "*.txt"
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS nodes)
endforeach()

# The testHdImagingShape test requires that we're using the legacy Pixar batch
# renderer and that the Viewport 2.0 render delegate is disabled.
mayaUsd_add_test(testHdImagingShape
    PYTHON_MODULE testHdImagingShape
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "MAYAUSD_DISABLE_VP2_RENDER_DELEGATE=1"
)
set_property(TEST testHdImagingShape APPEND PROPERTY LABELS nodes)
