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

#include "base/api.h"
#include "importTranslator.h"
#include "ProxyShape.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>

#include <mayaUsd/utils/undoHelperCommand.h>
#if defined(WANT_QT_BUILD)
#include <mayaUsd/ui/cmds/USDImportDialogCmd.h>
#endif

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>
#endif

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>
#include <maya/MDrawRegistry.h>

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Autodesk", "1.0", "Any");

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

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::initialize();
    if (!status) {
        status.perror("mayaUsdPlugin: unable to initialize ufe.");
    }
#endif

    status = plugin.registerShape(
        MayaUsd::ProxyShape::typeName,
        MayaUsd::ProxyShape::typeId,
        MayaUsd::ProxyShape::creator,
        MayaUsd::ProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        MayaUsdProxyShapePlugin::getProxyShapeClassification());
    CHECK_MSTATUS(status);

    status = UsdMayaUndoHelperCommand::initialize(plugin);
    if (!status) {
        status.perror(std::string("registerCommand ").append(
                          UsdMayaUndoHelperCommand::name()).c_str());
    }

#if defined(WANT_QT_BUILD)
    status = MayaUsd::USDImportDialogCmd::initialize(plugin);
    if (!status) {
        MString err("registerCommand" ); err += MayaUsd::USDImportDialogCmd::fsName;
        status.perror(err);
    }
#endif

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

    return status;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    MStatus status;

    status = UsdMayaUndoHelperCommand::finalize(plugin);
    if (!status) {
        status.perror(std::string("deregisterCommand ").append(
                          UsdMayaUndoHelperCommand::name()).c_str());
    }

#if defined(WANT_QT_BUILD)
    status = MayaUsd::USDImportDialogCmd::finalize(plugin);
    if (!status) {
        MString err("deregisterCommand" ); err += MayaUsd::USDImportDialogCmd::fsName;
        status.perror(err);
    }
#endif

    status = plugin.deregisterFileTranslator("USD Import");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister import translator.");
    }

    status = plugin.deregisterNode(MayaUsd::ProxyShape::typeId);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::finalize();
    CHECK_MSTATUS(status);
#endif

    return status;
}
