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
#include <sstream>

#include "adskExportCommand.h"
#include "adskImportCommand.h"
#include "adskListShadingModesCommand.h"

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>
#include <maya/MDrawRegistry.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>

#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/base/api.h>
#include <mayaUsd/commands/editTargetCommand.h>
#include <mayaUsd/commands/layerEditorCommand.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>
#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurfacePlugin.h>

#include "base/api.h"
#include "exportTranslator.h"
#include "importTranslator.h"
#include "ProxyShape.h"

#include <mayaUsd/utils/undoHelperCommand.h>
#if defined(WANT_QT_BUILD)
#include <mayaUsdUI/ui/USDImportDialogCmd.h>
#endif

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>
#endif

#if defined(MAYAUSD_VERSION)
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#else
#error "MAYAUSD_VERSION is not defined"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MStatus status;
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
        MAYAUSD_NS::UsdMayaExportTranslator::translatorName,
        "",
        MAYAUSD_NS::UsdMayaExportTranslator::creator,
        "mayaUsdTranslatorExport", // options script name
        const_cast<char*>(MAYAUSD_NS::UsdMayaExportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register export translator.");
    }

    registerCommandCheck<MAYAUSD_NS::ADSKMayaUSDExportCommand>(plugin);
    registerCommandCheck<MAYAUSD_NS::ADSKMayaUSDImportCommand>(plugin);
    registerCommandCheck<MAYAUSD_NS::EditTargetCommand>(plugin);
    registerCommandCheck<MAYAUSD_NS::LayerEditorCommand>(plugin);

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
    status = MAYAUSD_NS::ufe::initialize();
    if (!status) {
        status.perror("mayaUsdPlugin: unable to initialize ufe.");
    }
#endif

    status = plugin.registerShape(
        MAYAUSD_NS::ProxyShape::typeName,
        MAYAUSD_NS::ProxyShape::typeId,
        MAYAUSD_NS::ProxyShape::creator,
        MAYAUSD_NS::ProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        MayaUsdProxyShapePlugin::getProxyShapeClassification());
    CHECK_MSTATUS(status);

    registerCommandCheck<MAYAUSD_NS::ADSKMayaUSDListShadingModesCommand>(plugin);

    status = UsdMayaUndoHelperCommand::initialize(plugin);
    if (!status) {
        status.perror(std::string("registerCommand ").append(
                          UsdMayaUndoHelperCommand::name()).c_str());
    }

#if defined(WANT_QT_BUILD)
    status = MAYAUSD_NS::USDImportDialogCmd::initialize(plugin);
    if (!status) {
        MString err("registerCommand" ); err += MAYAUSD_NS::USDImportDialogCmd::fsName;
        status.perror(err);
    }
#endif

    status = PxrMayaUsdPreviewSurfacePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

    plugin.registerUI("mayaUsd_pluginUICreation", "mayaUsd_pluginUIDeletion", 
        "mayaUsd_pluginBatchLoad", "mayaUsd_pluginBatchUnload");

    // As of 2-Aug-2019, these PlugPlugin translators are not loaded
    // automatically.  To be investigated.  A duplicate of this code is in the
    // Pixar plugin.cpp.
    const std::vector<std::string> translatorPluginNames{
        "mayaUsd_Schemas", "mayaUsd_Translators"};
    const auto& plugRegistry = PlugRegistry::GetInstance();
    std::stringstream msg("mayaUsdPlugin: ");
    for (const auto& pluginName : translatorPluginNames) {
        auto plugin = plugRegistry.GetPluginWithName(pluginName);
        if (!plugin) {
            status = MStatus::kFailure;
            msg << "translator " << pluginName << " not found.";
            status.perror(msg.str().c_str());
        }
        else {
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
    MStatus status;

    status = PxrMayaUsdPreviewSurfacePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

    status = UsdMayaUndoHelperCommand::finalize(plugin);
    if (!status) {
        status.perror(std::string("deregisterCommand ").append(
                          UsdMayaUndoHelperCommand::name()).c_str());
    }

    deregisterCommandCheck<MAYAUSD_NS::ADSKMayaUSDListShadingModesCommand>(plugin);

#if defined(WANT_QT_BUILD)
    status = MAYAUSD_NS::USDImportDialogCmd::finalize(plugin);
    if (!status) {
        MString err("deregisterCommand" ); err += MAYAUSD_NS::USDImportDialogCmd::fsName;
        status.perror(err);
    }
#endif

    status = plugin.deregisterFileTranslator("USD Import");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister import translator.");
    }

    status = plugin.deregisterFileTranslator(MAYAUSD_NS::UsdMayaExportTranslator::translatorName);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister export translator.");
    }
    deregisterCommandCheck<MAYAUSD_NS::ADSKMayaUSDExportCommand>(plugin);
    deregisterCommandCheck<MAYAUSD_NS::ADSKMayaUSDImportCommand>(plugin);
    deregisterCommandCheck<MAYAUSD_NS::EditTargetCommand>(plugin);
    deregisterCommandCheck<MAYAUSD_NS::LayerEditorCommand>(plugin);
    status = plugin.deregisterNode(MAYAUSD_NS::ProxyShape::typeId);
    CHECK_MSTATUS(status);
    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
    status = MAYAUSD_NS::ufe::finalize();
    CHECK_MSTATUS(status);
#endif

    UsdMayaSceneResetNotice::RemoveListener();
    
    return status;
}
