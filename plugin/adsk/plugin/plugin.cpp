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
#include "adskExportCommand.h"
#include "adskImportCommand.h"
#include "adskListShadingModesCommand.h"
#include "base/api.h"
#include "exportTranslator.h"
#include "importTranslator.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/commands/editTargetCommand.h>
#include <mayaUsd/commands/layerEditorCommand.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>
#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/utils/undoHelperCommand.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/envSetting.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurfacePlugin.h>

#include <sstream>
#if defined(WANT_QT_BUILD)
#include <mayaUsdUI/ui/USDImportDialogCmd.h>
#endif

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdTransform3dCommonAPI.h>
#include <mayaUsd/ufe/UsdTransform3dFallbackMayaXformStack.h>
#include <mayaUsd/ufe/UsdTransform3dMatrixOp.h>
#include <mayaUsd/ufe/UsdTransform3dMayaXformStack.h>

#include <ufe/runTimeMgr.h>

// For Maya preview release 121 enabling and disabling of new Transform3d
// functionality.  PPT, 1-Dec-2020.
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#endif

#endif

#if defined(MAYAUSD_VERSION)
#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)
#else
#error "MAYAUSD_VERSION is not defined"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const MTypeId MayaUsdPreviewSurface_typeId(0x58000096);
const MString MayaUsdPreviewSurface_typeName("usdPreviewSurface");
const MString MayaUsdPreviewSurface_registrantId("mayaUsdPlugin");

#if defined(WANT_UFE_BUILD) && defined(UFE_V2_FEATURES_AVAILABLE)
// Keep a reference to the existing USD Transform3d handler, to restore on
// finalization.
Ufe::Transform3dHandler::Ptr g_OldTransform3dHandler;
Ufe::Transform3dHandler::Ptr g_NewTransform3dHandler;

class ToggleTransform3d : public MPxCommand
{
public:
    ToggleTransform3d() { }
    static void* creator() { return new ToggleTransform3d(); }

    static const MString commandName;

    MStatus doIt(const MArgList&) override { return redoIt(); }

    void toggle() const
    {
        bool toNew
            = (Ufe::RunTimeMgr::instance().transform3dHandler(MayaUsd::ufe::getUsdRunTimeId())
               == g_OldTransform3dHandler);
        Ufe::RunTimeMgr::instance().setTransform3dHandler(
            MayaUsd::ufe::getUsdRunTimeId(),
            toNew ? g_NewTransform3dHandler : g_OldTransform3dHandler);
        MGlobal::displayInfo(
            toNew ? MString("Using chain of responsibility Transform3d handlers.")
                  : MString("Using legacy Transform3d handler."));
    }

    MStatus redoIt() override
    {
        toggle();
        return MS::kSuccess;
    }
    MStatus undoIt() override
    {
        toggle();
        return MS::kSuccess;
    }
    bool isUndoable() const override { return true; }
};

const MString ToggleTransform3d::commandName { "toggleTransform3d" };

#endif

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

} // namespace

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceReader(MayaUsdPreviewSurface_typeName);
};
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceWriter(MayaUsdPreviewSurface_typeName);
};

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, "Autodesk", TOSTRING(MAYAUSD_VERSION), "Any");

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

    registerCommandCheck<MayaUsd::ADSKMayaUSDExportCommand>(plugin);
    registerCommandCheck<MayaUsd::ADSKMayaUSDImportCommand>(plugin);
    registerCommandCheck<MayaUsd::EditTargetCommand>(plugin);
    registerCommandCheck<MayaUsd::LayerEditorCommand>(plugin);

    status = plugin.registerCommand(
        MayaUsd::UsdUndoBlockCmd::commandName, MayaUsd::UsdUndoBlockCmd::creator);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::initialize();
    if (!status) {
        status.perror("mayaUsdPlugin: unable to initialize ufe.");
    }

#ifdef UFE_V2_FEATURES_AVAILABLE
    // Set up a chain of responsibility for Transform3d interface creation,
    // from least important to most important:
    // - Perform operations on a Maya transform stack appended to the existing
    //   transform stack (fallback).
    // - Perform operations on a 4x4 matrix transform op.
    // - Perform operations using the USD common transform API.
    // - Perform operations using a Maya transform stack.
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    auto  usdRtid = MayaUsd::ufe::getUsdRunTimeId();
    g_OldTransform3dHandler = runTimeMgr.transform3dHandler(usdRtid);
    auto fallbackHandler = MayaUsd::ufe::UsdTransform3dFallbackMayaXformStackHandler::create();
    auto matrixHandler = MayaUsd::ufe::UsdTransform3dMatrixOpHandler::create(fallbackHandler);
    auto commonAPIHandler = MayaUsd::ufe::UsdTransform3dCommonAPIHandler::create(matrixHandler);
    auto mayaStackHandler
        = MayaUsd::ufe::UsdTransform3dMayaXformStackHandler::create(commonAPIHandler);
    g_NewTransform3dHandler = mayaStackHandler;
    runTimeMgr.setTransform3dHandler(usdRtid, mayaStackHandler);

    status = plugin.registerCommand(ToggleTransform3d::commandName, ToggleTransform3d::creator);
    if (!status) {
        status.perror(
            MString("mayaUsdPlugin: unable to register command ") + ToggleTransform3d::commandName);
    }
#endif

#endif

    status = plugin.registerShape(
        MayaUsd::ProxyShape::typeName,
        MayaUsd::ProxyShape::typeId,
        MayaUsd::ProxyShape::creator,
        MayaUsd::ProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        MayaUsdProxyShapePlugin::getProxyShapeClassification());
    CHECK_MSTATUS(status);

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
        err += MayaUsd::USDImportDialogCmd::fsName;
        status.perror(err);
    }
#endif

    status = PxrMayaUsdPreviewSurfacePlugin::initialize(
        plugin,
        MayaUsdPreviewSurface_typeName,
        MayaUsdPreviewSurface_typeId,
        MayaUsdPreviewSurface_registrantId);
    CHECK_MSTATUS(status);

    plugin.registerUI(
        "mayaUsd_pluginUICreation",
        "mayaUsd_pluginUIDeletion",
        "mayaUsd_pluginBatchLoad",
        "mayaUsd_pluginBatchUnload");

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

    UsdMayaSceneResetNotice::InstallListener();

    return status;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    MStatus   status;

    status = PxrMayaUsdPreviewSurfacePlugin::finalize(
        plugin,
        MayaUsdPreviewSurface_typeName,
        MayaUsdPreviewSurface_typeId,
        MayaUsdPreviewSurface_registrantId);
    CHECK_MSTATUS(status);

    status = UsdMayaUndoHelperCommand::finalize(plugin);
    if (!status) {
        status.perror(
            std::string("deregisterCommand ").append(UsdMayaUndoHelperCommand::name()).c_str());
    }

    deregisterCommandCheck<MayaUsd::ADSKMayaUSDListShadingModesCommand>(plugin);

#if defined(WANT_QT_BUILD)
    status = MayaUsd::USDImportDialogCmd::finalize(plugin);
    if (!status) {
        MString err("deregisterCommand");
        err += MayaUsd::USDImportDialogCmd::fsName;
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
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDExportCommand>(plugin);
    deregisterCommandCheck<MayaUsd::ADSKMayaUSDImportCommand>(plugin);
    deregisterCommandCheck<MayaUsd::EditTargetCommand>(plugin);
    deregisterCommandCheck<MayaUsd::LayerEditorCommand>(plugin);

    status = plugin.deregisterNode(MayaUsd::ProxyShape::typeId);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

    status = plugin.deregisterCommand(MayaUsd::UsdUndoBlockCmd::commandName);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
#ifdef UFE_V2_FEATURES_AVAILABLE
    status = plugin.deregisterCommand(ToggleTransform3d::commandName);
    if (!status) {
        status.perror(
            MString("mayaUsdPlugin: unable to deregister command ")
            + ToggleTransform3d::commandName);
    }

    // Restore the initial maya-usd Transform3d handler.
    Ufe::RunTimeMgr::instance().setTransform3dHandler(
        MayaUsd::ufe::getUsdRunTimeId(), g_OldTransform3dHandler);

    g_OldTransform3dHandler = nullptr;
    g_NewTransform3dHandler = nullptr;
#endif

    status = MayaUsd::ufe::finalize();
    CHECK_MSTATUS(status);
#endif

    UsdMayaSceneResetNotice::RemoveListener();

    return status;
}
