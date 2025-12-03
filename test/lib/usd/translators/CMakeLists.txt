set(TEST_SCRIPT_FILES
    testUsdExport8BitNormalMap.py
    testUsdExportAndHide.py
    testUsdExportAnimation.py
    testUsdExportAsClip.py
    testUsdExportBindTransform.py
    testUsdExportBlendshapes.py
    testUsdExportCamera.py
    testUsdExportCameraAttrSpline.py
    testUsdExportCameraSameName.py
    testUsdExportColorSets.py
    testUsdExportConnected.py
    testUsdExportDefaultPrim.py
    testUsdExportDisplayColor.py
    testUsdExportEulerFilter.py
    testUsdExportFileFormat.py
    testUsdExportFilterTypes.py
    testUsdExportFrameOffset.py
    testUsdExportGeomSubset.py
    testUsdExportInstances.py
    testUsdExportLayerAttributes.py
    testUsdExportLight.py
    testUsdExportLightAttrSplines.py
    testUsdExportLocator.py
    testUsdExportMayaInstancer.py
    testUsdExportMesh.py
    testUsdExportNurbsCurve.py
    testUsdExportOpenLayer.py
    testUsdExportOverImport.py
    testUsdExportUsdPreviewSurface.py
    testUsdExportRootPrim.py
    testUsdExportTypes.py
    testUsdExportUnits.py
    testUsdExportUpAxis.py
    testUsdExportUVSetsSwitch.py

    # To investigate: following test asserts in MFnParticleSystem, but passes.
    # PPT, 17-Jun-20.
    testUsdExportParticles.py
    testUsdExportPref.py
    testUsdExportRenderLayerMode.py

    # # XXX: This test is disabled by default since it requires the RenderMan for Maya plugin.
    # testUsdExportRfMLight.py
    testUsdExportRoots.py
    testUsdExportRootsAndRootPrim.py
    testUsdExportSelection.py
    testUsdExportSelectionHierarchy.py
    testUsdExportSkeleton.py
    testUsdExportSkin.py
    testUsdExportSplineXforms.py
    testUsdExportStagesAsRefs.py
    testUsdExportStripNamespaces.py
    testUsdExportStroke.py
    testUsdExportTexture.py
    testUsdExportUserTaggedAttributes.py
    testUsdExportVisibilityDefault.py
    testUsdImportAnonymousLayer.py
    testUsdImportCamera.py
    testUsdImportBasisCurves.py
    testUsdImportBatchNurbsCurves.py
    testUsdImportBlendShapes.py
    testUsdImportLinearCurves.py
    testUsdImportHighDegreeNurbsCurves.py
    testUsdImportNurbsCurves.py
    testUsdImportBatchBezierCurves.py
    testUsdImportColorSets.py
    testUsdImportDisplacement.py
    testUsdImportExportScope.py
    testUsdImportExportTypelessDefs.py

    # To investigate: following test asserts in TDNshapeEditorManager.cpp, but
    # passes.  PPT, 17-Jun-20.
    testUsdImportFrameRange.py
    testUsdImportFramerates.py
    testUsdImportInstances.py
    testUsdImportLight.py
    testUsdImportMayaReference.py
    testUsdImportMesh.py
    testUsdImportPointCache.py
    testUsdImportPreviewSurface.py
    testUsdImportRelativeTextures.py

    # XXX: This test is disabled by default since it requires the RenderMan for Maya plugin.
    # testUsdImportRfMLight.py
    testUsdImportSessionLayer.py
    testUsdImportShadingModeDisplayColor.py
    testUsdImportShadingModePxrRis.py
    testUsdImportUVSetMappings.py
    testUsdImportUSDZTextures.py
    testUsdExportImportRoundtripPreviewSurface.py
    testUsdImportSkeleton.py
    testUsdImportUnits.py
    testUsdImportUpAxis.py
    testUsdImportXforms.py
    testUsdImportXformAnim.py
    testUsdImportEulerFilter.py
    testUsdMayaAdaptor.py
    testUsdMayaAdaptorGeom.py
    testUsdMayaAdaptorMetadata.py
    testUsdMayaAdaptorUndoRedo.py
)

# This test is disabled by default since it requires the LookdevX plugin
#if(MAYA_APP_VERSION VERSION_GREATER_EQUAL 2025)
#    list(APPEND TEST_SCRIPT_FILES        
#        testUsdExportMaterialXSurfaceShader.py
#    )
#endif()

if(BUILD_PXR_PLUGIN)
    # This test uses the file "UsdExportUVTransforms.ma" which
    # requires the plugin "pxrUsdPreviewSurface" that is built by the
    # Pixar plugin.
    list(APPEND TEST_SCRIPT_FILES
        testUsdExportUVTransforms.py
    )

    # This test uses the file "asset.ma" which
    # requires the plugin "pxrUsdPreviewSurface" that is built by the
    # Pixar plugin.
    list(APPEND TEST_SCRIPT_FILES
        testUsdExportUsdzPackage.py
    )
endif()

if (BUILD_RFM_TRANSLATORS)
    list(APPEND TEST_SCRIPT_FILES
        testUsdImportRfMShaders.py
    )
endif()

foreach(script ${TEST_SCRIPT_FILES})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME=1"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
    )

    # Add a ctest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS translators)
endforeach()

# These tests use custom material scope names and must not use
# the USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME env var because
# it takes precedence over the provided material scope name.
set(TEST_SCRIPT_FILES_WITH_MATERIAL_SCOPE
    testUsdExportDisplacement.py
    testUsdExportShadingInstanced.py
    testUsdExportShadingModePxrRis.py
    testUsdExportImportUDIM.py
)

if (BUILD_RFM_TRANSLATORS)
    list(APPEND TEST_SCRIPT_FILES_WITH_MATERIAL_SCOPE
        testUsdExportRfMShaders.py
    )
endif()

foreach(script ${TEST_SCRIPT_FILES_WITH_MATERIAL_SCOPE})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        PYTHON_MODULE ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
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
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdExportUVSets APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdExportUVSetsFloat
    PYTHON_MODULE testUsdExportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_WRITE_UV_AS_FLOAT2=1"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdExportUVSetsFloat APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdExportUVSetMappings
    PYTHON_MODULE testUsdExportUVSetMappings
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_WRITE_UV_AS_FLOAT2=0"
        "MAYAUSD_PROVIDE_DEFAULT_TEXCOORD_PRIMVAR_NAME=1"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdExportUVSetMappings APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportUVSets
    PYTHON_MODULE testUsdImportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_READ_FLOAT2_AS_UV=0"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdImportUVSets APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportUVSetsFloat
    PYTHON_MODULE testUsdImportUVSets
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "PIXMAYA_READ_FLOAT2_AS_UV=1"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdImportUVSetsFloat APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportChaser
    PYTHON_MODULE testUsdImportChaser
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdImportChaser APPEND PROPERTY LABELS translators)

set(CUSTOM_TEST_SCRIPT_FILES
    testUsdExportCustomConverter.template.py
    testUsdImportCustomConverter.template.py
)
set(PLUGIN_TYPES
    Maya
    USD
)
foreach(template_script ${CUSTOM_TEST_SCRIPT_FILES})
    string(REGEX REPLACE "\\.template.py" ".py" script
            "${template_script}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${template_script}"
        "${CMAKE_CURRENT_BINARY_DIR}/${script}"
        @ONLY
    )

    foreach(plugin_type ${PLUGIN_TYPES})
        mayaUsd_get_unittest_target(module ${script})
        set(target "${module}-${plugin_type}")
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${module}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
                "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_BINARY_DIR}/../plugin/${plugin_type}"
                "INPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
                "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}"
                "USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME=1"
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
                )
        set_property(TEST ${target} APPEND PROPERTY LABELS translators)
    endforeach()
endforeach()

# These import/export tests will work with older versions of Maya and USD.
# However getting the test materials to appear in the Maya viewport will
# require a recent Maya preview release combined with a bleeding edge USD
if (MAYA_APP_VERSION VERSION_GREATER 2022)
    set(MTLX_TEST_SCRIPT_FILES
        testUsdExportMaterialScope.py
        testUsdExportMaterialScopeLegacy.py
        testUsdExportMaterialsOnly.py
        testUsdExportAssignedMaterials.py
        testUsdExportMaterialX.py
        testUsdImportMaterialX.py
		testUsdExportFannedOutFileNodesMaterial.py
    )
    foreach(script ${MTLX_TEST_SCRIPT_FILES})
        mayaUsd_get_unittest_target(target ${script})
        mayaUsd_add_test(${target}
            PYTHON_MODULE ${target}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )

        # Add a ctest label to these tests for easy filtering.
        set_property(TEST ${target} APPEND PROPERTY LABELS translators)
    endforeach()

    if (BUILD_RFM_TRANSLATORS)
        mayaUsd_add_test(testUsdExportMultiMaterial
            PYTHON_MODULE testUsdExportMultiMaterial
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        )
        set_property(TEST testUsdExportMultiMaterial APPEND PROPERTY LABELS translators)
    endif()
endif()

# We are explicitly not setting PXR_PLUGINPATH_NAME here. We want to test
# manually loading the plugin that provides Maya export.
mayaUsd_add_test(testUsdMayaListShadingModesCommand
    PYTHON_MODULE testUsdMayaListShadingModesCommand
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdMayaListShadingModesCommand APPEND PROPERTY LABELS translators)

# Requires Physics API to test some exporters:
# One of the tests in this file requires UsdMaya (from the Pixar plugin). That test
# will be skipped if not found (probably because BUILD_PXR_PLUGIN is off).
mayaUsd_add_test(testUsdExportSchemaApi
    PYTHON_MODULE testUsdExportSchemaApi
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "MAYA_PLUG_IN_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugin"
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_BINARY_DIR}/../plugin/nullApiExporter"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdExportSchemaApi APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdImportExportSiteSpecificConfig
    PYTHON_MODULE testUsdImportExportSiteSpecificConfig
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_BINARY_DIR}/../plugin/SiteSpecificConfig"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
set_property(TEST testUsdImportExportSiteSpecificConfig APPEND PROPERTY LABELS translators)

mayaUsd_add_test(testUsdExportEmptyXforms
    PYTHON_MODULE testUsdExportEmptyXforms
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${CMAKE_CURRENT_BINARY_DIR}/../plugin/ExportEmptyConfig"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)
