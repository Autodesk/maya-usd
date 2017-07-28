set(PXR_PACKAGE AL_USDMayaTestPlugin)

list(APPEND AL_maya_test_headers
		AL/UnitTestHarness.h
        AL/maya/tests/CommandGuiHelperTest.h
        AL/maya/tests/NodeHelperUnitTest.h
        test_usdmaya.h
)
list(APPEND AL_maya_test_source
		AL/UnitTestHarness.cpp
        AL/maya/tests/CommandGuiHelperTest.cpp
        AL/maya/tests/NodeHelperUnitTest.cpp
        AL/usdmaya/commands/test_LayerCommands.cpp
        AL/usdmaya/commands/test_ProxyShapeSelect.cpp
        AL/usdmaya/commands/test_InternalProxySelection.cpp
        AL/usdmaya/nodes/test_ActiveInactive.cpp
        AL/usdmaya/nodes/test_HostDrivenTransforms.cpp
        AL/usdmaya/nodes/test_Layer.cpp
        AL/usdmaya/nodes/test_ProxyShape.cpp
        AL/usdmaya/nodes/test_TransformMatrix.cpp
        AL/usdmaya/nodes/test_USDToMayaMappingDB.cpp
        test_maya_MenuBuilder.cpp
        test_maya_NodeHelper.cpp
        test_translators_AnimationTranslator.cpp
        test_translators_CameraTranslator.cpp
        test_translators_DgTranslator.cpp
        test_translators_MeshTranslator.cpp
        test_translators_NurbsCurveTranslator.cpp
        test_translators_TransformTranslator.cpp
        test_translators_Translator.cpp
        test_usdmaya_AttributeType.cpp
        test_usdmaya_Utils.cpp
        test_usdmaya.cpp
)

add_definitions(-DAL_USDMAYA_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data")

add_definitions(
    -DAL_USDMAYA_UNITTEST
)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        AL_USDMaya
      	usdImaging
        usdImagingGL
        ${GTEST_LIBRARIES}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${GTEST_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        "../../../lib/AL_USDMaya"

    CPPFILES
        ${AL_maya_test_source}
        plugin.cpp
)

