find_package(GTest REQUIRED)

set(PXR_PACKAGE AL_USDMayaTestPlugin)

list(APPEND AL_maya_test_headers
    test_usdmaya.h
)

list(APPEND AL_maya_test_source
        plugin.cpp
        AL/maya/test_DiffCore.cpp
        AL/maya/test_EventHandler.cpp
        AL/maya/test_MatrixToSRT.cpp
        AL/maya/test_MayaEventManager.cpp
        AL/usdmaya/commands/test_ExportCommands.cpp
        AL/usdmaya/commands/test_LayerCommands.cpp
        AL/usdmaya/commands/test_ProxyShapeSelect.cpp
        AL/usdmaya/commands/test_InternalProxySelection.cpp
        AL/usdmaya/commands/test_ProxyShapeImport.cpp
        AL/usdmaya/commands/test_LayerManagerCommands.cpp
        AL/usdmaya/fileio/export_blendshape.cpp
        AL/usdmaya/fileio/export_constraints.cpp
        AL/usdmaya/fileio/export_ik.cpp
        AL/usdmaya/fileio/export_import_instancing.cpp
        AL/usdmaya/fileio/export_lattice.cpp
        AL/usdmaya/fileio/export_nonlinear.cpp
        AL/usdmaya/fileio/export_unmerged.cpp
        AL/usdmaya/fileio/export_multiple_shapes.cpp
        AL/usdmaya/fileio/import_instances.cpp
        AL/usdmaya/fileio/test_activeInActiveTranslators.cpp
        AL/usdmaya/nodes/test_ActiveInactive.cpp
        AL/usdmaya/nodes/test_LayerManager.cpp
        AL/usdmaya/nodes/test_ProxyShape.cpp
        AL/usdmaya/nodes/test_ProxyUsdGeomCamera.cpp
        AL/usdmaya/nodes/test_Transform.cpp
        AL/usdmaya/nodes/test_TransformMatrix.cpp
        AL/usdmaya/nodes/test_TranslatorContext.cpp
        AL/usdmaya/nodes/test_ExtraDataPlugin.cpp
        AL/usdmaya/nodes/test_ProxyShapeSelectabilityDB.cpp
        AL/usdmaya/nodes/proxy/test_PrimFilter.cpp
        AL/usdmaya/test_SelectabilityDB.cpp
        AL/usdmaya/test_DiffGeom.cpp
        AL/usdmaya/test_DiffPrimVar.cpp
        AL/usdmaya/commands/test_TranslateCommand.cpp
        test_translators_AnimationTranslator.cpp
        test_translators_CameraTranslator.cpp
        test_translators_DgTranslator.cpp
        test_translators_MeshTranslator.cpp
        test_translators_NurbsCurveTranslator.cpp
        test_translators_TransformTranslator.cpp
        test_translators_Translator.cpp
        test_translators_PythonBindings.cpp
        test_usdmaya_AttributeType.cpp
        test_usdmaya.cpp
)

add_library(${PXR_PACKAGE} ${AL_maya_test_source})

target_compile_definitions(${PXR_PACKAGE}
PRIVATE
    AL_USDMAYA_UNITTEST
    AL_USDMAYATEST_PLUGIN_EXPORT
    AL_USDMAYA_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
)

target_link_libraries(${PXR_PACKAGE}
    AL_EventSystem
    AL_USDMayaUtils
    AL_MayaUtils
    AL_USDUtils
    AL_USDMaya   #Adding this recursively pulls in lots of things
    AL_MayaTest
    AL_USDMayaSchemas
    AL_USDMayaSchemasTest
    ${GTEST_LIBRARIES}
)

target_include_directories(${PXR_PACKAGE}
PRIVATE
    ${GTEST_INCLUDE_DIRS}
    ${MAYA_INCLUDE_DIRS}
    ${MAYAUSD_INCLUDE_DIR}
    "../../lib/AL_USDMaya"    
    "../../schemas"
    ${MAYATEST_INCLUDE_LOCATION}
)

if(MSVC)
    set_target_properties(${PXR_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(APPLE)
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            SUFFIX ".bundle"
            MACOSX_RPATH TRUE
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "@loader_path/../lib;@loader_path/../../pxr/lib;@loader_path/../../pxr/maya/lib;${PXR_USD_LOCATION}/lib"
    )
else()
    set_target_properties(${PXR_PACKAGE}
        PROPERTIES
            PREFIX ""
            INSTALL_RPATH_USE_LINK_PATH TRUE
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../../pxr/lib:$ORIGIN/../../pxr/maya/lib:${PXR_USD_LOCATION}/lib:${PXR_USD_LOCATION}/lib64"
    )
endif()

install(TARGETS
    ${PXR_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)

add_test(
    NAME TestAL_USDMaya
    COMMAND
        ${CMAKE_CURRENT_SOURCE_DIR}/run_mayaplugin_tests.sh
            ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX}
            ${USD_LIBRARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT SKIP_USDMAYA_TESTS)
    add_subdirectory(py)
endif()