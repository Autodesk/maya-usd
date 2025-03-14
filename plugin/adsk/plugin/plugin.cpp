//
// Copyright 2019 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "ProxyShape.h"
#include "ProxyShapeListener.h"
#include "adskExportCommand.h"
#include "adskImportCommand.h"
#include "adskListJobContextsCommand.h"
#include "adskListShadingModesCommand.h"
#include "adskStageLoadUnloadCommands.h"
#include "base/api.h"
#include "exportTranslator.h"
#include "geomNode.h"
#include "gizmoGeometryOverride.h"
#include "gizmoShape.h"
#include "importTranslator.h"
#include "mayaUsdInfoCommand.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/commands/editTargetCommand.h>
#include <mayaUsd/commands/layerEditorCommand.h>
#include <mayaUsd/commands/layerEditorWindowCommand.h>
#include <mayaUsd/commands/schemaCommand.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>
#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsd/utils/diagnosticDelegate.h>
#include <mayaUsd/utils/undoHelperCommand.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/envSetting.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>
#include <maya/MUserData.h>
#include <ufe/runTimeMgr.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurfacePlugin.h>
#if HAS_LOOKDEVXUSD
#include <lookdevXUsd/LookdevXUsd.h>
#endif // HAS_LOOKDEVXUSD

#include <sstream>

#if defined(WANT_QT_BUILD)
#include <mayaUsdUI/ui/USDImportDialogCmd.h>
#include <mayaUsdUI/ui/initStringResources.h>
#include <mayaUsdUI/ui/layerEditorCommands.h>
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
#include "adskMaterialCommands.h"

#include <mayaUsd/commands/PullPushCommands.h>
#include <mayaUsd/fileio/primUpdaterManager.h>
#endif

#if defined(WANT_QT_BUILD)
#include <mayaUsdUI/ui/batchSaveLayersUIDelegate.h>
#endif

#if defined(MAYAUSD_VERSION)
#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)
#else
#error "MAYAUSD_VERSION is not defined"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const MTypeId kMayaUsdPreviewSurface_typeId(0x58000096);
const MString kMayaUsdPreviewSurface_typeName("usdPreviewSurface");
const MString kMayaUsdPreviewSurface_registrantId("mayaUsdPlugin");

const MString kMayaUsdPlugin_registrantId("mayaUsdPlugin");

template <typename T> void registerCommandCheck(MFnPlugin& plugin)
{
    auto status = plugin.registerCommand(T::commandName, T::creator, T::createSyntax);
    if (!status) {
        status.perror(MString("mayaUsdPlugin: unable to register command ") + T::commandName);
    }
}

template <typename T> void deregisterCommandCheck(MFnPlugin& plugin)
{
    auto status = plugin.deregisterCommand(T::commandName);
    if (!status) {
        status.perror(MString("mayaUsdPlugin: unable to deregister command ") + T::commandName);
    }
}

MStatus registerStringResources()
{
    MStatus status { MStatus::MStatusCode::kSuccess };
#if defined(WANT_QT_BUILD)
    status = MayaUsd::initStringResources();
#endif
    return status;
}

} // namespace

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceReader(kMayaUsdPreviewSurface_typeName);
};
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceWriter(kMayaUsdPreviewSurface_typeName);
};

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, "Autodesk", TOSTRING(MAYAUSD_VERSION), "Any");

    // register string resources
    status = plugin.registerUIStrings(registerStringResources, "mayaUSDRegisterStrings");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register string resources.");
    }

    status = plugin.registerFileTranslator(
        "USD Import",
        "",
        UsdMayaImportTranslator::creator,
        "mayaUsdTranslatorImport", // options script name
        const_cast<char*>(UsdMayaImportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register import translator.");
    }

    status = plugin.registerFileTranslator(
        MayaUsd::UsdMayaExportTranslator::translatorName,
        "",
        MayaUsd::UsdMayaExportTranslator::creator,
        "mayaUsdTranslatorExport", // options script name
        const_cast<char*>(MayaUsd::UsdMayaExportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register export translator.");
    }

    registerCommandCheck<MayaUsd::ADSKMayaUsdStageLoadAllCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUsdStageUnloadAllCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDExportCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDImportCommand>(plugin);
    registerCommandCheck<MayaUsd::EditTargetCommand>(plugin);
    registerCommandCheck<MayaUsd::LayerEditorCommand>(plugin);
    registerCommandCheck<UsdLayerEditor::GetSelectedWidgetLayersMpxCommand>(plugin);
    registerCommandCheck<UsdLayerEditor::SetSelectedWidgetLayersMpxCommand>(plugin);
    registerCommandCheck<MayaUsd::SchemaCommand>(plugin);
    registerCommandCheck<MayaUsd::MayaUsdInfoCommand>(plugin);
#if defined(WANT_QT_BUILD)
    registerCommandCheck<MayaUsd::LayerEditorWindowCommand>(plugin);
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
    registerCommandCheck<MayaUsd::ufe::EditAsMayaCommand>(plugin);
    registerCommandCheck<MayaUsd::ufe::MergeToUsdCommand>(plugin);
    registerCommandCheck<MayaUsd::ufe::DiscardEditsCommand>(plugin);
    registerCommandCheck<MayaUsd::ufe::DuplicateCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDGetMaterialsForRenderersCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDGetMaterialsInStageCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDMaterialBindingsCommand>(plugin);
#endif

    status = plugin.registerCommand(
        MayaUsd::MayaUsdUndoBlockCmd::commandName, MayaUsd::MayaUsdUndoBlockCmd::creator);
    CHECK_MSTATUS(status);

    MGlobal::executePythonCommand(
        "try:\n"
        "    from ufe_ae.usd.nodes.usdschemabase import collectionMayaHost\n"
        "    collectionMayaHost.registerCommands('mayaUsdPlugin')\n"
        "except:\n"
        "    pass\n");

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

    status = MayaUsd::ufe::initialize();
    if (!status) {
        status.perror("mayaUsdPlugin: unable to initialize ufe.");
    }

#if HAS_LOOKDEVXUSD
    LookdevXUsd::initialize();
#endif // HAS_LOOKDEVXUSD

    status = plugin.registerShape(
        MayaUsd::ProxyShape::typeName,
        MayaUsd::ProxyShape::typeId,
        MayaUsd::ProxyShape::creator,
        MayaUsd::ProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        MayaUsdProxyShapePlugin::getProxyShapeClassification());
    CHECK_MSTATUS(status);

    status = plugin.registerNode(
        MayaUsd::ProxyShapeListener::typeName,
        MayaUsd::ProxyShapeListener::typeId,
        MayaUsd::ProxyShapeListener::creator,
        MayaUsd::ProxyShapeListener::initialize);
    CHECK_MSTATUS(status);

    status = plugin.registerNode(
        MayaUsd::MayaUsdGeomNode::typeName,
        MayaUsd::MayaUsdGeomNode::typeId,
        MayaUsd::MayaUsdGeomNode::creator,
        MayaUsd::MayaUsdGeomNode::initialize);
    CHECK_MSTATUS(status);

    status = plugin.registerShape(
        MayaUsd::GizmoShape::typeName,
        MayaUsd::GizmoShape::id,
        MayaUsd::GizmoShape::creator,
        MayaUsd::GizmoShape::initialize,
        nullptr,
        &MayaUsd::GizmoGeometryOverride::dbClassification);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::registerGeometryOverrideCreator(
        MayaUsd::GizmoGeometryOverride::dbClassification,
        kMayaUsdPlugin_registrantId,
        MayaUsd::GizmoGeometryOverride::Creator);
    CHECK_MSTATUS(status);

    registerCommandCheck<MayaUsd::ADSKMayaUSDListJobContextsCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDListShadingModesCommand>(plugin);

    status = UsdMayaUndoHelperCommand::initialize(plugin);
    if (!status) {
        status.perror(
            std::string("registerCommand ").append(UsdMayaUndoHelperCommand::name()).c_str());
    }

#if defined(WANT_QT_BUILD)
    status = MayaUsd::USDImportDialogCmd::initialize(plugin);
    if (!status) {
        MString err("registerCommand");
        err += MayaUsd::USDImportDialogCmd::name;
        status.perror(err);
    }
#endif

    status = PxrMayaUsdPreviewSurfacePlugin::initialize(
        plugin,
        kMayaUsdPreviewSurface_typeName,
        kMayaUsdPreviewSurface_typeId,
        kMayaUsdPreviewSurface_registrantId);
    CHECK_MSTATUS(status);

    plugin.registerUI(
        "mayaUsd_pluginUICreation",
        "mayaUsd_pluginUIDeletion",
        "mayaUsd_pluginBatchLoad",
        "mayaUsd_pluginBatchUnload");

    // Register to file path editor
    status = MGlobal::executeCommand("filePathEditor -registerType \"mayaUsdProxyShape.filePath\" "
                                     "-typeLabel \"mayaUsdProxyShape.filePath\" -temporary");
    CHECK_MSTATUS(status);

    // As of 2-Aug-2019, these PlugPlugin translators are not loaded
    // automatically.  To be investigated.  A duplicate of this code is in the
    // Pixar plugin.cpp.
    const std::vector<std::string> translatorPluginNames { "mayaUsd_Schemas",
                                                           "mayaUsd_Translators" };
    const auto&                    plugRegistry = PlugRegistry::GetInstance();
    std::stringstream              msg("mayaUsdPlugin: ");
    for (const auto& pluginName : translatorPluginNames) {
        auto plugin = plugRegistry.GetPluginWithName(pluginName);
        if (!plugin) {
            status = MStatus::kFailure;
            msg << "translator " << pluginName << " not found.";
            status.perror(msg.str().c_str());
        } else {
            // Load is a no-op if already loaded.
            if (!plugin->Load()) {
                status = MStatus::kFailure;
                msg << pluginName << " translator load failed.";
                status.perror(msg.str().c_str());
            }
        }
    }

    MayaUsd::LayerManager::addSupportForNodeType(MayaUsd::ProxyShape::typeId);
#if defined(WANT_QT_BUILD)
    UsdLayerEditor::initialize();
    MayaUsd::LayerManager::SetBatchSaveDelegate(UsdLayerEditor::batchSaveLayersUIDelegate);
#endif

    UsdMayaSceneResetNotice::InstallListener();
    UsdMayaBeforeSceneResetNotice::InstallListener();
    UsdMayaExitNotice::InstallListener();
    UsdMayaDiagnosticDelegate::InstallDelegate();

#ifdef UFE_V3_FEATURES_AVAILABLE
    // Install notifications
    PrimUpdaterManager::getInstance();
#endif

    return status;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    MStatus   status;

    status = PxrMayaUsdPreviewSurfacePlugin::finalize(
        plugin,
        kMayaUsdPreviewSurface_typeName,
        kMayaUsdPreviewSurface_typeId,
        kMayaUsdPreviewSurface_registrantId);
    CHECK_MSTATUS(status);

    status = UsdMayaUndoHelperCommand::finalize(plugin);
    if (!status) {
        status.perror(
            std::string("deregisterCommand ").append(UsdMayaUndoHelperCommand::name()).c_str());
    }

    deregisterCommandCheck<MayaUsd::ADSKMayaUSDListShadingModesCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDListJobContextsCommand>(plugin);

#if defined(WANT_QT_BUILD)
    status = MayaUsd::USDImportDialogCmd::finalize(plugin);
    if (!status) {
        MString err("deregisterCommand");
        err += MayaUsd::USDImportDialogCmd::name;
        status.perror(err);
    }
#endif

    status = plugin.deregisterFileTranslator("USD Import");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister import translator.");
    }

    status = plugin.deregisterFileTranslator(MayaUsd::UsdMayaExportTranslator::translatorName);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister export translator.");
    }
    deregisterCommandCheck<MayaUsd::ADSKMayaUsdStageLoadAllCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUsdStageUnloadAllCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDExportCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDImportCommand>(plugin);
    deregisterCommandCheck<MayaUsd::EditTargetCommand>(plugin);
    deregisterCommandCheck<MayaUsd::LayerEditorCommand>(plugin);
    deregisterCommandCheck<MayaUsd::SchemaCommand>(plugin);
    deregisterCommandCheck<MayaUsd::MayaUsdInfoCommand>(plugin);
#if defined(WANT_QT_BUILD)
    deregisterCommandCheck<MayaUsd::LayerEditorWindowCommand>(plugin);
    MayaUsd::LayerEditorWindowCommand::cleanupOnPluginUnload();
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
    deregisterCommandCheck<MayaUsd::ufe::EditAsMayaCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ufe::MergeToUsdCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ufe::DiscardEditsCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ufe::DuplicateCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDGetMaterialsForRenderersCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDGetMaterialsInStageCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDMaterialBindingsCommand>(plugin);
#endif

    status = plugin.deregisterNode(MayaUsd::ProxyShapeListener::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(MayaUsd::ProxyShape::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(MayaUsd::MayaUsdGeomNode::typeId);
    CHECK_MSTATUS(status);

    plugin.deregisterNode(MayaUsd::GizmoShape::id);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::deregisterGeometryOverrideCreator(
        MayaUsd::GizmoGeometryOverride::dbClassification, kMayaUsdPlugin_registrantId);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

    status = plugin.deregisterCommand(MayaUsd::MayaUsdUndoBlockCmd::commandName);
    CHECK_MSTATUS(status);

    MGlobal::executePythonCommand(
        "try:\n"
        "    from ufe_ae.usd.nodes.usdschemabase import collectionMayaHost\n"
        "    collectionMayaHost.deregisterCommands('mayaUsdPlugin')\n"
        "except:\n"
        "    pass\n");

    // Deregister from file path editor
    status
        = MGlobal::executeCommand("filePathEditor -deregisterType \"mayaUsdProxyShape.filePath\" "
                                  "-typeLabel \"mayaUsdProxyShape.filePath\" -temporary");
    CHECK_MSTATUS(status);

    MGlobal::executeCommand("mayaUSDUnregisterStrings()");

#if HAS_LOOKDEVXUSD
    LookdevXUsd::uninitialize();
#endif // HAS_LOOKDEVXUSD

    status = MayaUsd::ufe::finalize();
    CHECK_MSTATUS(status);

    MayaUsd::LayerManager::removeSupportForNodeType(MayaUsd::ProxyShape::typeId);
#if defined(WANT_QT_BUILD)
    MayaUsd::LayerManager::SetBatchSaveDelegate(nullptr);
#endif

    UsdMayaSceneResetNotice::RemoveListener();
    UsdMayaBeforeSceneResetNotice::RemoveListener();
    UsdMayaExitNotice::RemoveListener();
    UsdMayaDiagnosticDelegate::RemoveDelegate();

    return status;
}
