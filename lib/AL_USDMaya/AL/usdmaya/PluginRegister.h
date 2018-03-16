//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"
#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/MenuBuilder.h"
#include "AL/usdmaya/Global.h"
#include "AL/usdmaya/StageData.h"
#include "AL/usdmaya/DrivenTransformsData.h"
#include "AL/usdmaya/cmds/DebugCommands.h"
#include "AL/usdmaya/cmds/EventCommand.h"
#include "AL/usdmaya/cmds/LayerCommands.h"
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"
#include "AL/usdmaya/cmds/ProxyShapeSelectCommands.h"
#include "AL/usdmaya/cmds/UnloadPrim.h"
#include "AL/usdmaya/fileio/Export.h"
#include "AL/usdmaya/fileio/ExportTranslator.h"
#include "AL/usdmaya/fileio/Import.h"
#include "AL/usdmaya/fileio/ImportTranslator.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyDrawOverride.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/ProxyShapeUI.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include "maya/MDrawRegistry.h"
#include "maya/MGlobal.h"
#include "maya/MStatus.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This method is basically the main initializePlugin routine. The reason for it being a template is simply
///         a historical artifact.
/// \param  plugin the MFnPlugin
/// \todo   Move this code into initializePlugin
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
template<typename AFnPlugin>
MStatus registerPlugin(AFnPlugin& plugin)
{
  GlfGlewInit();

  if(!MGlobal::optionVarExists("AL_usdmaya_selectMode"))
  {
    MGlobal::setOptionVarValue("AL_usdmaya_selectMode", 0);
  }

  if(!MGlobal::optionVarExists("AL_usdmaya_selectResolution"))
  {
    MGlobal::setOptionVarValue("AL_usdmaya_selectResolution", 10);
  }

  MStatus status;

  // gpuCachePluginMain used as an example.
  if (MGlobal::kInteractive == MGlobal::mayaState()) {
    const auto otherPriority = MSelectionMask::getSelectionTypePriority("polymesh");
    if (!MSelectionMask::registerSelectionType(AL::usdmaya::nodes::ProxyShape::s_selectionMaskName, otherPriority)) {
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

  AL_REGISTER_DATA(plugin, AL::usdmaya::StageData);
  AL_REGISTER_DATA(plugin, AL::usdmaya::DrivenTransformsData);
  AL_REGISTER_COMMAND(plugin, AL::maya::utils::CommandGuiListGen);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCreateLayer);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerGetLayers);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerCurrentEditTarget);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSave);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::LayerSetMuted);
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
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::Callback);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::TriggerEvent);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::DeleteCallbacks);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::CallbackQuery);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::Event);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventQuery);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::EventLookup);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::cmds::TranslatePrim);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::fileio::ImportCommand);
  AL_REGISTER_COMMAND(plugin, AL::usdmaya::fileio::ExportCommand);
  AL_REGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ImportTranslator);
  AL_REGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ExportTranslator);
  AL_REGISTER_DRAW_OVERRIDE(plugin, AL::usdmaya::nodes::ProxyDrawOverride);
  AL_REGISTER_SHAPE_NODE(plugin, AL::usdmaya::nodes::ProxyShape, AL::usdmaya::nodes::ProxyShapeUI, AL::usdmaya::nodes::ProxyDrawOverride);
  AL_REGISTER_TRANSFORM_NODE(plugin, AL::usdmaya::nodes::Transform, AL::usdmaya::nodes::TransformationMatrix);
  AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::Layer);
  // Since AL_MAYA_DECLARE_NODE / AL_MAYA_DEFINE_NODE declare/define "creator"
  // method, and AL_REGISTER_DEPEND_NODE registers "creator", in order to
  // define custom creator, need to either 'override' one of those... chose to
  // replace AL_REGISTER_DEPEND_NODE
  //AL_REGISTER_DEPEND_NODE(plugin, AL::usdmaya::nodes::LayerManager);
  {
    MStatus status = plugin.registerNode(
        AL::usdmaya::nodes::LayerManager::kTypeName,
        AL::usdmaya::nodes::LayerManager::kTypeId,
        AL::usdmaya::nodes::LayerManager::conditionalCreator,
        AL::usdmaya::nodes::LayerManager::initialise);
    if(!status) {
      status.perror("unable to register depend node LayerManager");
      return status;
    }
  }

  // generate the menu GUI + option boxes
  AL::usdmaya::cmds::constructLayerCommandGuis();
  AL::usdmaya::cmds::constructProxyShapeCommandGuis();
  AL::usdmaya::cmds::constructDebugCommandGuis();

  CHECK_MSTATUS(AL::maya::utils::MenuBuilder::generatePluginUI(plugin, "AL_usdmaya"));
  AL::usdmaya::Global::onPluginLoad();
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This method is basically the main uninitializePlugin routine. The reason for it being a template is simply
///         a historical artifact.
/// \param  plugin the MFnPlugin
/// \todo   Move this code into uninitializePlugin
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
template<typename AFnPlugin>
MStatus unregisterPlugin(AFnPlugin& plugin)
{
  MStatus status;

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

    if (!MSelectionMask::deregisterSelectionType(AL::usdmaya::nodes::ProxyShape::s_selectionMaskName)) {
      status.perror("Error deregistering selection mask!");
      return MS::kFailure;
    }
  }

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
  AL_UNREGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ImportTranslator);
  AL_UNREGISTER_TRANSLATOR(plugin, AL::usdmaya::fileio::ExportTranslator);
  AL_UNREGISTER_DRAW_OVERRIDE(plugin, AL::usdmaya::nodes::ProxyDrawOverride);
  AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::ProxyShape);
  AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::Transform);
  AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::Layer);
  AL_UNREGISTER_NODE(plugin, AL::usdmaya::nodes::LayerManager);
  AL_UNREGISTER_DATA(plugin, AL::usdmaya::DrivenTransformsData);
  AL_UNREGISTER_DATA(plugin, AL::usdmaya::StageData);

  AL::usdmaya::Global::onPluginUnload();
  return status;
}

#undef AL_RUN_TESTS

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
