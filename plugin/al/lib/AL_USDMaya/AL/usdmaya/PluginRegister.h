//
// Copyright 2017 Animal Logic
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
#pragma once
#include <pxr/pxr.h>
#if PXR_VERSION < 2102
#include <pxr/imaging/glf/glew.h>
#else
#include <pxr/imaging/garch/glApi.h>
#endif

#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/MenuBuilder.h"
#include "AL/usdmaya/Global.h"
#include "AL/usdmaya/cmds/CreateUsdPrim.h"
#include "AL/usdmaya/cmds/DebugCommands.h"
#include "AL/usdmaya/cmds/EventCommand.h"
#include "AL/usdmaya/cmds/LayerCommands.h"
#include "AL/usdmaya/cmds/ListTranslators.h"
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"
#include "AL/usdmaya/cmds/RendererCommands.h"
#include "AL/usdmaya/cmds/SyncFileIOGui.h"
#include "AL/usdmaya/cmds/UnloadPrim.h"
#include "AL/usdmaya/fileio/Export.h"
#include "AL/usdmaya/fileio/ExportTranslator.h"
#include "AL/usdmaya/fileio/Import.h"
#include "AL/usdmaya/fileio/ImportTranslator.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/MeshAnimCreator.h"
#include "AL/usdmaya/nodes/MeshAnimDeformer.h"
#include "AL/usdmaya/nodes/ProxyDrawOverride.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/ProxyShapeUI.h"
#include "AL/usdmaya/nodes/ProxyUsdGeomCamera.h"
#include "AL/usdmaya/nodes/RendererManager.h"
#include "AL/usdmaya/nodes/Scope.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include <basePxrUsdPreviewSurface/usdPreviewSurfacePlugin.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/glContext.h>

#include <maya/MDrawRegistry.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

#if defined(WANT_UFE_BUILD)
#include <mayaUsd/ufe/Global.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Short term fix to enable meshes to connect directly to USD prims. This will be removed
/// once the plugin
///         translator API has been updated to allow custom import/export options.
//----------------------------------------------------------------------------------------------------------------------
static const char* const g_geom_deformer_code = R"(
global proc AL_usdmaya_meshStaticImport()
{
  string $sl[] = `ls -sl`;
  for($s in $sl)
  {
    string $tm;
    string $mesh;
    string $nt = `nodeType $s`;
    if($nt == "mesh")
    {
      $ps = `listRelatives -p "AL_usdmaya_Transform" -type $s`;
      if(!size($ps)) 
        continue;
      $tm = $ps[0];
      $mesh = $s;
    }
    else
    if($nt == "AL_usdmaya_Transform")
    {
      $cs = `listRelatives -c -type "mesh" $s`;
      if(!size($cs)) 
        continue;
      $mesh = $cs[0];
      $tm = $s;
    }
    else
      continue;
            
    string $pp = `getAttr ($tm + ".primPath")`;
    string $cs[] = `listConnections -s 1 ($tm + ".inStageData")`; 
    if(!size($pp) || !size($cs))
      continue;
    if(size(`listConnections -s 1 ($mesh + ".inMesh")`))
      continue;
    
    $ctor = `createNode "AL_usdmaya_MeshAnimCreator"`;
    setAttr -type "string" ($ctor + ".primPath") $pp;
    connectAttr ($cs[0] + ".outStageData") ($ctor + ".inStageData");
    connectAttr ($ctor + ".outMesh") ($mesh + ".inMesh");
  }
}
global proc AL_usdmaya_meshAnimImport()
{
  string $sl[] = `ls -sl`;
  for($s in $sl)
  {
    string $tm;
    string $mesh;
    string $nt = `nodeType $s`;
    if($nt == "mesh")
    {
      $ps = `listRelatives -p "AL_usdmaya_Transform" -type $s`;
      if(!size($ps)) 
        continue;
      $tm = $ps[0];
      $mesh = $s;
    }
    else
    if($nt == "AL_usdmaya_Transform")
    {
      $cs = `listRelatives -c -type "mesh" $s`;
      if(!size($cs)) 
        continue;
      $mesh = $cs[0];
      $tm = $s;
    }
    else
      continue;
            
    string $pp = `getAttr ($tm + ".primPath")`;
    string $cs[] = `listConnections -s 1 ($tm + ".inStageData")`; 
    if(!size($pp) || !size($cs))
      continue;
    if(size(`listConnections -s 1 ($mesh + ".inMesh")`))
      continue;
    
    $ctor = `createNode "AL_usdmaya_MeshAnimCreator"`;
    $def = `createNode "AL_usdmaya_MeshAnimDeformer"`;
    setAttr -type "string" ($ctor + ".primPath") $pp;
    setAttr -type "string" ($def + ".primPath") $pp;
    connectAttr "time1.outTime" ($def + ".inTime");
    connectAttr ($cs[0] + ".outStageData") ($ctor + ".inStageData");
    connectAttr ($cs[0] + ".outStageData") ($def + ".inStageData");
    connectAttr ($ctor + ".outMesh") ($def + ".inMesh");
    connectAttr ($def + ".outMesh") ($mesh + ".inMesh");
  }
}
)";

// Type name for registration of USD preview surface
const MString AL_USDMAYA_PREVIEW_SURFACE_TYPENAME("alUsdPreviewSurface");

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This method is basically the main initializePlugin routine. The reason for it being a
/// template is simply
///         a historical artifact.
/// \param  plugin the MFnPlugin
/// \todo   Move this code into initializePlugin
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
template <typename AFnPlugin> MStatus registerPlugin(AFnPlugin& plugin)
{
#if PXR_VERSION < 2102
    GlfGlewInit();
#else
    GarchGLApiLoad();
#endif

    // We may be in a non-gui maya... if so,
    // GlfContextCaps::InitInstance() will error
    if (GlfGLContext::GetCurrentGLContext()->IsValid()) {
        GlfContextCaps::InitInstance();
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_selectMode")) {
        MGlobal::setOptionVarValue("AL_usdmaya_selectMode", 0);
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_selectResolution")) {
        MGlobal::setOptionVarValue("AL_usdmaya_selectResolution", 10);
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_pickMode")) {
        MGlobal::setOptionVarValue(
            "AL_usdmaya_pickMode", static_cast<int>(nodes::ProxyShape::PickMode::kPrims));
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_readAnimatedValues")) {
        MGlobal::setOptionVarValue("AL_usdmaya_readAnimatedValues", false);
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_selectionEnabled")) {
        MGlobal::setOptionVarValue("AL_usdmaya_selectionEnabled", true);
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_pushToPrim")) {
        MGlobal::setOptionVarValue("AL_usdmaya_pushToPrim", true);
    }

    if (!MGlobal::optionVarExists("AL_usdmaya_ignoreLockPrims")) {
        MGlobal::setOptionVarValue("AL_usdmaya_ignoreLockPrims", false);
    }

    MStatus status;

    // gpuCachePluginMain used as an example.
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        const auto otherPriority = MSelectionMask::getSelectionTypePriority("polymesh");
        if (!MSelectionMask::registerSelectionType(
                AL::usdmaya::nodes::ProxyShape::s_selectionMaskName, otherPriority)) {
            status.perror("Error registering selection mask!");
            return MS::kFailure;
        }

        MString cmd = "addSelectTypeItem(\"Surface\",\"";
        cmd += AL::usdmaya::nodes::ProxyShape::s_selectionMaskName;
        cmd += "\",\"";
        cmd += "AL Proxy Shape";
        cmd += "\")";

        status = MGlobal::executeCommand(cmd);
        if (!status) {
            status.perror("Error adding al_ProxyShape selection type!");
            return status;
        }
    }

    AL_REGISTER_COMMAND(plugin, AL::maya::utils::CommandGuiListGen);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::CreateUsdPrim);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCreateLayer);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerGetLayers);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCurrentEditTarget);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSave);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSetMuted);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ManageRenderer);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImport);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeFindLoadable);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImportAllTransforms);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeRemoveAllTransforms);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeResync);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImportPrimPathAsMaya);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapePrintRefCountState);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ChangeVariant);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ActivatePrim);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeSelect);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapePostSelect);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::InternalProxyShapeSelect);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::UsdDebugCommand);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListEvents);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListCallbacks);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListTranslators);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::Callback);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::TriggerEvent);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::DeleteCallbacks);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::CallbackQuery);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::Event);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventQuery);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventLookup);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::TranslatePrim);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerManager);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::SyncFileIOGui);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::fileio::ImportCommand);
    AL_REGISTER_COMMAND(plugin, AL::usdmaya::fileio::ExportCommand);
    AL_REGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ImportTranslator);
    AL_REGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ExportTranslator);
    AL_REGISTER_DRAW_OVERRIDE(plugin, AL::usdmaya::nodes::ProxyDrawOverride);

    status = MayaUsdProxyShapePlugin::initialize(plugin);
    CHECK_MSTATUS(status);

    status = PxrMayaUsdPreviewSurfacePlugin::initialize(
        plugin,
        AL_USDMAYA_PREVIEW_SURFACE_TYPENAME,
        AL_USDMAYA_USDPREVIEWSURFACE,
        AL_USDMAYA_PLUGIN_REGISTRANT_ID);
    CHECK_MSTATUS(status);

    if (MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        status = plugin.registerShape(
            AL::usdmaya::nodes::ProxyShape::kTypeName,
            AL::usdmaya::nodes::ProxyShape::kTypeId,
            AL::usdmaya::nodes::ProxyShape::creator,
            AL::usdmaya::nodes::ProxyShape::initialise,
            AL::usdmaya::nodes::ProxyShapeUI::creator,
            MayaUsdProxyShapePlugin::getProxyShapeClassification());
        CHECK_MSTATUS(status);
    } else {
        AL_REGISTER_SHAPE_NODE(
            plugin,
            AL::usdmaya::nodes::ProxyShape,
            AL::usdmaya::nodes::ProxyShapeUI,
            AL::usdmaya::nodes::ProxyDrawOverride);
    }

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::initialize();
    if (!status) {
        status.perror("Unable to initialize ufe.");
    }
#endif

    AL_REGISTER_TRANSFORM_NODE(
        plugin, AL::usdmaya::nodes::Scope, AL::usdmaya::nodes::BasicTransformationMatrix);
    AL_REGISTER_TRANSFORM_NODE(
        plugin, AL::usdmaya::nodes::Transform, AL::usdmaya::nodes::TransformationMatrix);
    AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::RendererManager);
    AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::Layer);
    AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::MeshAnimCreator);
    AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::MeshAnimDeformer);
    AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::ProxyUsdGeomCamera);

    // Since AL_MAYA_DECLARE_NODE / AL_MAYA_DEFINE_NODE declare/define "creator"
    // method, and AL_REGISTER_DEPEND_NODE registers "creator", in order to
    // define custom creator, need to either 'override' one of those... chose to
    // replace AL_REGISTER_DEPEND_NODE
    // AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::LayerManager);
    {
        MStatus status = plugin.registerNode(
            AL::usdmaya::nodes::LayerManager::kTypeName,
            AL::usdmaya::nodes::LayerManager::kTypeId,
            AL::usdmaya::nodes::LayerManager::conditionalCreator,
            AL::usdmaya::nodes::LayerManager::initialise);
        if (!status) {
            status.perror("unable to register depend node LayerManager");
            return status;
        }
    }

    // generate the menu GUI + option boxes
    AL::usdmaya::cmds::constructLayerCommandGuis();
    AL::usdmaya::cmds::constructProxyShapeCommandGuis();
    AL::usdmaya::cmds::constructDebugCommandGuis();
    AL::usdmaya::cmds::constructRendererCommandGuis();
    AL::usdmaya::cmds::constructPickModeCommandGuis();

    MGlobal::executeCommand(g_geom_deformer_code);
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Animated Geometry/Connect selected meshes to USD (static)",
        "AL_usdmaya_meshStaticImport");
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Animated Geometry/Connect selected meshes to USD (animated)",
        "AL_usdmaya_meshAnimImport");
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Selection Enabled",
        "optionVar -iv \\\"AL_usdmaya_selectionEnabled\\\" #1",
        true,
        MGlobal::optionVarIntValue("AL_usdmaya_selectionEnabled"));
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Enable pushToPrim",
        "optionVar -iv \\\"AL_usdmaya_pushToPrim\\\" #1",
        true,
        MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"));
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Selection Ignore Lock Prims Enabled",
        "optionVar -iv \\\"AL_usdmaya_ignoreLockPrims\\\" #1",
        true,
        MGlobal::optionVarIntValue("AL_usdmaya_ignoreLockPrims"));
    CHECK_MSTATUS(AL::maya::utils::MenuBuilder::generatePluginUI(plugin, "AL_usdmaya"));
    AL::usdmaya::Global::onPluginLoad();

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

    // Force all plugins to be loaded at startup time. Unless we load plugins upfront
    // options will not be registered until the start of import or export, and won't be available in
    // the GUI
    const TfType& translatorType = TfType::Find<AL::usdmaya::fileio::translators::TranslatorBase>();
    PlugPluginPtrVector plugins = PlugRegistry::GetInstance().GetAllPlugins();
    for (auto& plugin : plugins) {
        if (!plugin->IsLoaded() && plugin->DeclaresType(translatorType, true))
            plugin->Load();
    }

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This method is basically the main uninitializePlugin routine. The reason for it being a
/// template is simply
///         a historical artifact.
/// \param  plugin the MFnPlugin
/// \todo   Move this code into uninitializePlugin
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
template <typename AFnPlugin> MStatus unregisterPlugin(AFnPlugin& plugin)
{
    MStatus status;

#if defined(WANT_UFE_BUILD)
    status = MayaUsd::ufe::finalize();
    CHECK_MSTATUS(status);
#endif

    // gpuCachePluginMain used as an example.
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        MString cmd = "deleteSelectTypeItem(\"Surface\",\"";
        cmd += AL::usdmaya::nodes::ProxyShape::s_selectionMaskName;
        cmd += "\")";

        status = MGlobal::executeCommand(cmd);
        if (!status) {
            status.perror("Error removing al_ProxyShape selection type!");
            return status;
        }

        if (!MSelectionMask::deregisterSelectionType(
                AL::usdmaya::nodes::ProxyShape::s_selectionMaskName)) {
            status.perror("Error deregistering selection mask!");
            return MS::kFailure;
        }
    }

    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::SyncFileIOGui);
    AL_UNREGISTER_COMMAND(plugin, AL::maya::utils::CommandGuiListGen);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::InternalProxyShapeSelect);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapePostSelect);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeSelect);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ActivatePrim);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ChangeVariant);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCreateLayer);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCurrentEditTarget);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerGetLayers);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSave);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSetMuted);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ManageRenderer);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImport);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeFindLoadable);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImportAllTransforms);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeRemoveAllTransforms);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeResync);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapeImportPrimPathAsMaya);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ProxyShapePrintRefCountState);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::Callback);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListCallbacks);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListEvents);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::ListTranslators);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::TriggerEvent);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::DeleteCallbacks);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::CallbackQuery);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::Event);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventQuery);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventLookup);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::UsdDebugCommand);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::fileio::ImportCommand);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::fileio::ExportCommand);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::TranslatePrim);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerManager);
    AL_UNREGISTER_COMMAND(plugin, AL::usdmaya::cmds::CreateUsdPrim);
    AL_UNREGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ImportTranslator);
    AL_UNREGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ExportTranslator);
    AL_UNREGISTER_DRAW_OVERRIDE(plugin, AL::usdmaya::nodes::ProxyDrawOverride);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::MeshAnimDeformer);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::MeshAnimCreator);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::ProxyShape);

    status = PxrMayaUsdPreviewSurfacePlugin::finalize(
        plugin,
        AL_USDMAYA_PREVIEW_SURFACE_TYPENAME,
        AL_USDMAYA_USDPREVIEWSURFACE,
        AL_USDMAYA_PLUGIN_REGISTRANT_ID);
    CHECK_MSTATUS(status);

    status = MayaUsdProxyShapePlugin::finalize(plugin);
    CHECK_MSTATUS(status);

    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::Transform);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::Scope);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::RendererManager);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::Layer);
    AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::LayerManager);

    AL::usdmaya::Global::onPluginUnload();
    return status;
}

#undef AL_RUN_TESTS

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
