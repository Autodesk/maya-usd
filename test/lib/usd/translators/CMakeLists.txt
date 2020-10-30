set(TARGET_NAME IMPORT_EXPORT_TEST)

set(TEST_SCRIPT_FILES
    testUsdExportAsClip.py
    testUsdExportCamera.py
    testUsdExportColorSets.py
    testUsdExportConnected.py
    testUsdExportDisplacement.py
    testUsdExportDisplayColor.py
    testUsdExportEulerFilter.py
    testUsdExportFileFormat.py
    testUsdExportFilterTypes.py
    testUsdExportFrameOffset.py
    testUsdExportGeomSubset.py
    testUsdExportInstances.py
    testUsdExportLocator.py
    testUsdExportMayaInstancer.py
    testUsdExportMesh.py
    testUsdExportNurbsCurve.py
    testUsdExportOpenLayer.py
    testUsdExportOverImport.py
    testUsdExportParentScope.py
    # To investigate: following test asserts in MFnParticleSystem, but passes.
    # PPT, 17-Jun-20.
    testUsdExportParticles.py
    testUsdExportPref.py
    # Following test runs only in legacy render layer mode, otherwise skipped.
    # Legacy render layers are deprecated, and should not be used.
    # To avoid skipping and run in legacy render layer mode,
    # export MAYA_ENABLE_LEGACY_RENDER_LAYERS=1
    testUsdExportRenderLayerMode.py
    ## XXX: This test is disabled by default since it requires the RenderMan for Maya plugin.
    # testUsdExportRfMLight.py
    testUsdExportSelection.py
    testUsdExportSelectionHierarchy.py
    testUsdExportShadingInstanced.py
    testUsdExportShadingModePxrRis.py
    testUsdExportSkeleton.py
    testUsdExportStripNamespaces.py
    testUsdExportStroke.py
    testUsdExportUVSetMappings.py
    testUsdExportVisibilityDefault.py
    testUsdImportCamera.py
    testUsdImportColorSets.py
    testUsdImportDisplacement.py
    testUsdImportExportScope.py
    testUsdImportExportTypelessDefs.py
    # To investigate: following test asserts in TDNshapeEditorManager.cpp, but
    # passes.  PPT, 17-Jun-20.
    testUsdImportFrameRange.py
    testUsdImportInstances.py
    testUsdImportMayaReference.py
    testUsdImportMesh.py
    testUsdImportPreviewSurface.py
    # XXX: This test is disabled by default since it requires the RenderMan for Maya plugin.
    # testUsdImportRfMLight.py
    testUsdImportSessionLayer.py
    testUsdImportShadingModeDisplayColor.py
    testUsdImportShadingModePxrRis.py
    testUsdExportImportRoundtripPreviewSurface.py
    testUsdImportSkeleton.py
    testUsdImportXforms.py
    testUsdMayaAdaptor.py
    testUsdMayaAdaptorGeom.py
    testUsdMayaAdaptorMetadata.py
    testUsdMayaAdaptorUndoRedo.py
)

if (BUILD_RFM_TRANSLATORS)
    list(APPEND TEST_SCRIPT_FILES
        testUsdExportRfMShaders.py
        testUsdImportRfMShaders.py
    )
endif()

add_custom_target(${TARGET_NAME} ALL)

# copy all the resources and python scripts to build directory
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
    set_property(TEST ${target} APPEND PROPERTY LABELS translators)
endforeach()

# testUsdExportUVSets and testUsdImportUVSets are run twice, with float writing
# and reading (respectively) turned on and off.

mayaUsd_add_test(testUsdExportUVSets
    PYTHON_MODULE testUsdExportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_WRITE_UV_AS_FLOAT2=0"
)
set_property(TEST testUsdExportUVSets APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdExportUVSetsFloat
    PYTHON_MODULE testUsdExportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_WRITE_UV_AS_FLOAT2=1"
)
set_property(TEST testUsdExportUVSetsFloat APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportUVSets
    PYTHON_MODULE testUsdImportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_READ_FLOAT2_AS_UV=0"
)
set_property(TEST testUsdImportUVSets APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportUVSetsFloat
    PYTHON_MODULE testUsdImportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_READ_FLOAT2_AS_UV=1"
)
set_property(TEST testUsdImportUVSetsFloat APPEND PROPERTY LABELS translators)

# Test using standardSurface, which was introduced in Maya 2020.
if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2020)
    set(CUSTOM_TEST_SCRIPT_FILES
        testUsdExportCustomConverter.py
        testUsdImportCustomConverter.py
    )
    foreach(script ${CUSTOM_TEST_SCRIPT_FILES})
        mayaUsd_get_unittest_target(target ${script})
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
                "PXR_PLUGINPATH_NAME=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
        )
        set_property(TEST ${target} APPEND PROPERTY LABELS translators)
    endforeach()
endif()

# We are explicitly not setting PXR_PLUGINPATH_NAME here. We want to test
# manually loading the plugin that provides Maya export.
mayaUsd_add_test(testUsdMayaListShadingModesCommand
    PYTHON_MODULE testUsdMayaListShadingModesCommand
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
)
set_property(TEST testUsdMayaListShadingModesCommand APPEND PROPERTY LABELS translators)
