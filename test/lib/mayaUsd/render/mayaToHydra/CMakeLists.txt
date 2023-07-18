# Unit test scripts.
set(TEST_SCRIPT_FILES
    testMtohCommand.py
# Fail as of 22-Feb-2023.  Entered as MAYA-127898.
#    testMtohBasicRender.py
#    testMtohDagChanges.py
    testMeshes.py
    testNamespaces.py
    testVisibility.py
    testStageAddPrim.py
    cpp/testCppFramework.py
    cpp/testMayaSceneFlattening.py
    cpp/testMayaUsdUfeItems.py
)

# Test use of mesh adapter code for mesh support, using environment variable.
# As of 13-Jun-2023, if a test in this list is identical to a test in
# TEST_SCRIPT_FILES, it cannot write to an output directory, because running
# both versions of the test in parallel will conflict.
set(TEST_SCRIPT_FILES_MESH_ADAPTER
    testMeshes.py
)

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "IMAGE_DIFF_TOOL=${IMAGE_DIFF_TOOL}"

            # LD_LIBRARY_PATH needs to be set for the idiff executable because its 
            # RPATH is absolute rather than relative to ORIGIN, meaning the RPATH 
            # points to the absolute path on the machine where idiff was built.
            # This absence of relative paths for RPATH comes from OpenImageIO.
            # We introduce a second workaround to avoid Maya using usd's libpng, 
            # because both use incompatible versions of libpng. This is done by 
            # setting LD_LIBRARY_PATH to IDIFF_LD_LIBRARY_PATH only when we run 
            # idiff using Python's subprocess module.
            "IDIFF_LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}:${PXR_USD_LOCATION}/lib64:${PXR_USD_LOCATION}/lib"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Fallback to old color management. We will have to investigate
            # and introduce OCIOv2 compatible version of these tests.
            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS mayaHydra)
endforeach()

# Use mesh adapter for mesh support instead of MRenderItem.
foreach(script ${TEST_SCRIPT_FILES_MESH_ADAPTER})
    mayaUsd_get_unittest_target(target ${script})
    set(target "${target}_meshAdapter")
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"

            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            "MAYA_COLOR_MANAGEMENT_SYNCOLOR=1"

            "MAYA_HYDRA_USE_MESH_ADAPTER=1"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS mayaHydra)
endforeach()

# C++ unit tests
add_subdirectory(cpp)