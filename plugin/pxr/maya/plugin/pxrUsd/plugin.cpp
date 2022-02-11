//
// Copyright 2016 Pixar
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
#include "usdMaya/exportCommand.h"
#include "usdMaya/exportTranslator.h"
#include "usdMaya/importCommand.h"
#include "usdMaya/importTranslator.h"
#include "usdMaya/listShadingModesCommand.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/referenceAssembly.h"

#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>
#include <mayaUsd/utils/diagnosticDelegate.h>
#include <mayaUsd/utils/undoHelperCommand.h>

#include <pxr/pxr.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>
#endif

#include "api.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>

PXR_NAMESPACE_USING_DIRECTIVE

static const MString _RegistrantId("pxrUsdPlugin");

PXRUSD_API
MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, "Pixar", "1.0", "Any");

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::initialize();
    if (!status) {
        status.perror("Unable to initialize ufe.");
    }
#endif

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerShape(
        UsdMayaProxyShape::typeName,
        UsdMayaProxyShape::typeId,
        UsdMayaProxyShape::creator,
        UsdMayaProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        MayaUsdProxyShapePlugin::getProxyShapeClassification());
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerNode(
        UsdMayaReferenceAssembly::typeName,
        UsdMayaReferenceAssembly::typeId,
        UsdMayaReferenceAssembly::creator,
        UsdMayaReferenceAssembly::initialize,
        MPxNode::kAssembly,
        &UsdMayaReferenceAssembly::_classification);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::sourceFile("usdMaya.mel");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set the label for the assembly node type so that it appears correctly
    // in the 'Create -> Scene Assembly' menu.
    const MString assemblyTypeLabel("UsdReferenceAssembly");
    MString       setLabelCmd;
    status = setLabelCmd.format(
        "assembly -e -type ^1s -label ^2s", UsdMayaReferenceAssembly::typeName, assemblyTypeLabel);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MGlobal::executeCommand(setLabelCmd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Procs stored in usdMaya.mel
    // Add assembly callbacks for accessing data without creating an MPxAssembly instance
    status = MGlobal::executeCommand(
        "assembly -e -repTypeLabelProc usdMaya_UsdMayaReferenceAssembly_repTypeLabel -type "
        + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = MGlobal::executeCommand(
        "assembly -e -listRepTypesProc usdMaya_UsdMayaReferenceAssembly_listRepTypes -type "
        + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Attribute Editor Templates
    MString attribEditorCmd("from pxr.UsdMaya import AEpxrUsdReferenceAssemblyTemplate\n"
                            "AEpxrUsdReferenceAssemblyTemplate.addMelFunctionStubs()");
    status = MGlobal::executePythonCommand(attribEditorCmd);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plugin.registerCommand(
        "usdExport", PxrMayaUSDExportCommand::creator, PxrMayaUSDExportCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdExport");
    }

    status = plugin.registerCommand(
        "usdImport", PxrMayaUSDImportCommand::creator, PxrMayaUSDImportCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdImport");
    }

    status = plugin.registerCommand(
        "usdListShadingModes",
        PxrMayaUSDListShadingModesCommand::creator,
        PxrMayaUSDListShadingModesCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdListShadingModes");
    }

    status = UsdMayaUndoHelperCommand::initialize(plugin);
    if (!status) {
        status.perror(
            std::string("registerCommand ").append(UsdMayaUndoHelperCommand::name()).c_str());
    }

    status = plugin.registerFileTranslator(
        "pxrUsdImport",
        "",
        UsdMayaImportTranslator::creator,
        "usdTranslatorImport", // options script name
        const_cast<char*>(UsdMayaImportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("pxrUsd: unable to register USD Import translator.");
    }

    status = plugin.registerFileTranslator(
        "pxrUsdExport",
        "",
        UsdMayaExportTranslator::creator,
        "usdTranslatorExport", // options script name
        const_cast<char*>(UsdMayaExportTranslator::GetDefaultOptions().c_str()),
        true);
    if (!status) {
        status.perror("pxrUsd: unable to register USD Export translator.");
    }

    UsdMayaSceneResetNotice::InstallListener();
    UsdMayaBeforeSceneResetNotice::InstallListener();
    UsdMayaExitNotice::InstallListener();
    UsdMayaDiagnosticDelegate::InstallDelegate();

    // As of 2-Aug-2019, these PlugPlugin translators are not loaded
    // automatically.  To be investigated.  A duplicate of this code is in the
    // Autodesk plugin.cpp.
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

    return status;
}

PXRUSD_API
MStatus uninitializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj);

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::finalize();
    CHECK_MSTATUS(status);
#endif

    status = plugin.deregisterCommand("usdImport");
    if (!status) {
        status.perror("deregisterCommand usdImport");
    }

    status = plugin.deregisterCommand("usdExport");
    if (!status) {
        status.perror("deregisterCommand usdExport");
    }

    status = plugin.deregisterCommand("usdListShadingModes");
    if (!status) {
        status.perror("deregisterCommand usdListShadingModes");
    }

    status = UsdMayaUndoHelperCommand::finalize(plugin);
    if (!status) {
        status.perror(
            std::string("deregisterCommand ").append(UsdMayaUndoHelperCommand::name()).c_str());
    }

    status = plugin.deregisterFileTranslator("pxrUsdImport");
    if (!status) {
        status.perror("pxrUsd: unable to deregister USD Import translator.");
    }

    status = plugin.deregisterFileTranslator("pxrUsdExport");
    if (!status) {
        status.perror("pxrUsd: unable to deregister USD Export translator.");
    }

    status
        = MGlobal::executeCommand("assembly -e -deregister " + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaReferenceAssembly::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaProxyShape::typeId);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

    UsdMayaSceneResetNotice::RemoveListener();
    UsdMayaBeforeSceneResetNotice::RemoveListener();
    UsdMayaExitNotice::RemoveListener();
    UsdMayaDiagnosticDelegate::RemoveDelegate();

    return status;
}
