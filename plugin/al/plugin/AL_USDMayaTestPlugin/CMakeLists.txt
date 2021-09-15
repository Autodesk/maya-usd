set(TARGET_NAME AL_USDMayaTestPlugin)

find_package(GTest REQUIRED)

add_library(${TARGET_NAME} SHARED)

target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        AL/maya/test_EventHandler.cpp
        AL/maya/test_MatrixToSRT.cpp
        AL/maya/test_MayaEventManager.cpp
        AL/usdmaya/commands/test_ExportCommands.cpp
        AL/usdmaya/commands/test_InternalProxySelection.cpp
        AL/usdmaya/commands/test_LayerCommands.cpp
        AL/usdmaya/commands/test_LayerManagerCommands.cpp
        AL/usdmaya/commands/test_ProxyShapeImport.cpp
        AL/usdmaya/commands/test_ProxyShapeSelect.cpp
        AL/usdmaya/commands/test_TranslateCommand.cpp
        AL/usdmaya/fileio/export_blendshape.cpp
        AL/usdmaya/fileio/export_constraints.cpp
        AL/usdmaya/fileio/export_ik.cpp
        AL/usdmaya/fileio/export_import_instancing.cpp
        AL/usdmaya/fileio/export_lattice.cpp
        AL/usdmaya/fileio/export_multiple_shapes.cpp
        AL/usdmaya/fileio/export_nonlinear.cpp
        AL/usdmaya/fileio/export_unmerged.cpp
        AL/usdmaya/fileio/import_instances.cpp
        AL/usdmaya/fileio/import_playback_range.cpp
        AL/usdmaya/fileio/test_activeInActiveTranslators.cpp
        AL/usdmaya/nodes/proxy/test_PrimFilter.cpp
        AL/usdmaya/nodes/test_ActiveInactive.cpp
        AL/usdmaya/nodes/test_ExtraDataPlugin.cpp
        AL/usdmaya/nodes/test_LayerManager.cpp
        AL/usdmaya/nodes/test_lockPrims.cpp
        AL/usdmaya/nodes/test_ProxyShape.cpp
        AL/usdmaya/nodes/test_ProxyShapeSelectabilityDB.cpp
        AL/usdmaya/nodes/test_ProxyUsdGeomCamera.cpp
        AL/usdmaya/nodes/test_Transform.cpp
        AL/usdmaya/nodes/test_TransformMatrix.cpp
        AL/usdmaya/nodes/test_ScopeMatrix.cpp
        AL/usdmaya/nodes/test_TranslatorContext.cpp
        AL/usdmaya/nodes/test_VariantFallbacks.cpp
        AL/usdmaya/test_DiffGeom.cpp
        AL/usdmaya/test_DiffPrimVar.cpp
        AL/usdmaya/test_SelectabilityDB.cpp
        test_translators_AnimationTranslator.cpp
        test_translators_CameraTranslator.cpp
        test_translators_DgTranslator.cpp
        test_translators_MeshTranslator.cpp
        test_translators_NurbsCurveTranslator.cpp
        test_translators_PythonBindings.cpp
        test_translators_TransformTranslator.cpp
        test_translators_Translator.cpp
        test_usdmaya.cpp
        test_usdmaya_AttributeType.cpp
)

if(IS_WINDOWS)
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".mll")
elseif(IS_MACOSX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" SUFFIX ".bundle")
elseif(IS_LINUX)
    set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")
endif()

# compiler configuration
mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        AL_USDMAYA_UNITTEST
        AL_USDMAYATEST_PLUGIN_EXPORT
        AL_USDMAYA_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${GTEST_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        "../../lib/AL_USDMaya"
        "../../schemas"
        ${MAYATEST_INCLUDE_LOCATION}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        AL_EventSystem
        AL_USDMayaUtils
        AL_MayaUtils
        mayaUsdUtils
        AL_USDMaya   #Adding this recursively pulls in lots of things
        AL_MayaTest
        AL_USDMayaSchemas
        AL_USDMayaSchemasTest
        ${GTEST_LIBRARIES}
)

# handle run-time search paths
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "plugin")
    mayaUsd_add_rpath(rpath "../lib")
    if(BUILD_TESTS)
        mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/plugin/pxr/maya/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

# install
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${AL_INSTALL_PREFIX}/plugin
    RUNTIME
    DESTINATION ${AL_INSTALL_PREFIX}/plugin
)

# unit tests

set(pycode "
import sys
import maya.cmds
maya.cmds.loadPlugin('AL_USDMayaTestPlugin')
sys.exit(maya.cmds.AL_maya_test_UnitTestHarness())
")

mayaUsd_add_test(${TARGET_NAME}
    PYTHON_COMMAND "${pycode}"
    ENV
        "${PXR_OVERRIDE_PLUGINPATH_NAME}=${ADDITIONAL_PXR_PLUGINPATH_NAME}"
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
)

if(NOT SKIP_USDMAYA_TESTS)
    add_subdirectory(py)
endif()
