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
#include "AL/maya/CommandGuiHelper.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/cmds/LayerCommands.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/LayerVisitor.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include "maya/MArgDatabase.h"
#include "maya/MDagPath.h"
#include "maya/MFnDagNode.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyGraph.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MSelectionList.h"
#include "maya/MStringArray.h"
#include "maya/MSyntax.h"

#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usdUtils/stageCache.h"
#include "pxr/usd/sdf/listOp.h"

#include <sstream>

namespace AL {
namespace usdmaya {
namespace cmds {
//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerCommandBase::setUpCommonSyntax()
{
  MSyntax syntax;
  syntax.useSelectionAsDefault(true);
  syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
  syntax.addFlag("-p", "-proxy", MSyntax::kString);
  return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MArgDatabase LayerCommandBase::makeDatabase(const MArgList& args)
{
  MStatus status;
  MArgDatabase database(syntax(), args, &status);
  if(!status)
    throw status;
  return database;
}

//----------------------------------------------------------------------------------------------------------------------
nodes::ProxyShape* LayerCommandBase::getShapeNode(const MArgDatabase& args)
{
  MDagPath path;
  MSelectionList sl;
  args.getObjects(sl);

  for(uint32_t i = 0; i < sl.length(); ++i)
  {
    MStatus status = sl.getDagPath(i, path);

    if(path.node().hasFn(MFn::kTransform))
    {
      path.extendToShape();
    }

    if(path.node().hasFn(MFn::kPluginShape))
    {
      MFnDagNode fn(path);
      if(fn.typeId() == nodes::ProxyShape::kTypeId)
      {
        return (nodes::ProxyShape*)fn.userNode();
      }
    }
  }
  sl.clear();

  {
    if(args.isFlagSet("-p"))
    {
      MString proxyName;
      if(args.getFlagArgument("-p", 0, proxyName))
      {
        sl.add(proxyName);
        if(sl.length())
        {
          MStatus status = sl.getDagPath(0, path);

          if(path.node().hasFn(MFn::kTransform))
          {
            path.extendToShape();
          }

          if(path.node().hasFn(MFn::kPluginShape))
          {
            MFnDagNode fn(path);
            if(fn.typeId() == nodes::ProxyShape::kTypeId)
            {
              return (nodes::ProxyShape*)fn.userNode();
            }
          }
        }
      }
      MGlobal::displayError("Invalid ProxyShape specified/selected with -p flag");
    }
    else
      MGlobal::displayError("No ProxyShape specified/selected");
  }

  throw MS::kFailure;
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
MObject LayerCommandBase::getSelectedNode(const MArgDatabase& args, const MTypeId typeId)
{
  MSelectionList sl;
  args.getObjects(sl);


  MFnDependencyNode fn;
  MObject obj;
  for(uint32_t i = 0; i < sl.length(); ++i)
  {
    sl.getDependNode(i, obj);
    fn.setObject(obj);
    if(fn.typeId() == typeId)
      return obj;
  }
  return MObject::kNullObj;
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr LayerCommandBase::getShapeNodeStage(const MArgDatabase& args)
{
  nodes::ProxyShape* node = getShapeNode(args);
  return node ? node->getUsdStage() : UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
// LayerGetLayers
//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(LayerGetLayers, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerGetLayers::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  syn.addFlag("-u", "-used", MSyntax::kNoArg);
  syn.addFlag("-m", "-muted", MSyntax::kNoArg);
  syn.addFlag("-s", "-stack", MSyntax::kNoArg);
  syn.addFlag("-sl", "-sessionLayer", MSyntax::kNoArg);
  syn.addFlag("-rl", "-rootLayer", MSyntax::kNoArg);
  syn.addFlag("-mn", "-mayaNames", MSyntax::kNoArg);
  syn.addFlag("-hi", "-hierarchy", MSyntax::kNoArg);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerGetLayers::isUndoable() const
{
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerGetLayers::doIt(const MArgList& argList)
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerGetLayers::doIt\n");
  try
  {
    MArgDatabase args = makeDatabase(argList);
    AL_MAYA_COMMAND_HELP(args, g_helpText);

    nodes::ProxyShape* proxyShape = getShapeNode(args);
    UsdStageRefPtr stage = proxyShape->usdStage();
    MStringArray results;

    auto push = [](MStringArray& results, const MString& newLayer)
    {
      for(uint32_t i = 0; i < results.length(); ++i)
      {
        if(results[i] == newLayer) return;
      }
      results.append(newLayer);
    };

    const bool useMayaNames = args.isFlagSet("-mn");
    if(args.isFlagSet("-rl"))
    {

      SdfLayerHandle rootLayer = stage->GetRootLayer();
      if(useMayaNames)
      {
        setResult(proxyShape->findLayerMayaName(rootLayer));
      }
      else
      {
        setResult(convert(rootLayer->GetDisplayName()));
      }
      return MS::kSuccess;
    }
    else
    if(args.isFlagSet("-m"))
    {
      if(useMayaNames)
      {
        MGlobal::displayError("Cannot query many names on muted layers (they layers haven't been imported into Maya)");
        return MS::kFailure;
      }
      const std::vector<std::string>& layers = stage->GetMutedLayers();
      for(auto it = layers.begin(); it != layers.end(); ++it)
      {
        results.append(convert(*it));
      }
    }
    else
    if(args.isFlagSet("-s"))
    {
      const bool includeSessionLayer = args.isFlagSet("-sl");
      SdfLayerHandleVector layerStack = stage->GetLayerStack(includeSessionLayer);
      for(auto it = layerStack.begin(); it != layerStack.end(); ++it)
      {
        if(useMayaNames)
        {
          results.append(proxyShape->findLayerMayaName(*it));
        }
        else
        {
          results.append(convert((*it)->GetDisplayName()));
        }
      }
    }
    else
    if(args.isFlagSet("-u"))
    {
      const bool includeSessionLayer = args.isFlagSet("-sl");
      SdfLayerHandle sessionLayer = stage->GetSessionLayer();
      SdfLayerHandleVector layerStack = stage->GetUsedLayers();
      for(auto it = layerStack.begin(); it != layerStack.end(); ++it)
      {
        if(!includeSessionLayer)
        {
          if(sessionLayer == *it)
            continue;
        }
        if(useMayaNames)
        {
          results.append(proxyShape->findLayerMayaName(*it));
        }
        else
        {
          results.append(convert((*it)->GetDisplayName()));
        }
      }
    }
    else
    if(args.isFlagSet("-sl"))
    {
      SdfLayerHandle sessionLayer = stage->GetSessionLayer();
      if(useMayaNames)
      {
        setResult(proxyShape->findLayerMayaName(sessionLayer));
      }
      else
      {
        setResult(convert(sessionLayer->GetDisplayName()));
      }
      return MS::kSuccess;
    }
    else
    if(args.isFlagSet("-hi"))
    {
      class HierarchyBuilder : public nodes::LayerVisitor
      {
      public:
        HierarchyBuilder(nodes::ProxyShape* shape, bool mayaNames)
        : nodes::LayerVisitor(shape), m_mayaNames(mayaNames) {}

        void onVisit()
        {
          nodes::Layer* layer = thisLayer();
          MString item;
          for(uint32_t i = 1; i < depth(); ++i)
          {
            item += "  ";
          }
          if(m_mayaNames)
          {
            MFnDependencyNode fn(layer->thisMObject());
            item += fn.name();
          }
          else
          {
            LAYER_HANDLE_CHECK(layer->getHandle());
            item += convert(layer->getHandle()->GetDisplayName());
          }
          result.append(item);
        }

        MStringArray result;
        bool m_mayaNames;
      };

      HierarchyBuilder builder(proxyShape, useMayaNames);
      builder.visitAll();
      setResult(builder.result);
      return MS::kSuccess;
    }
    setResult(results);
  }
  catch(const MStatus& status)
  {
    return status;
  }
  return MS::kSuccess;
}


//----------------------------------------------------------------------------------------------------------------------
// LayerCreateLayer
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerCreateLayer, AL_usdmaya);

MSyntax LayerCreateLayer::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.addFlag("-o", "-open", MSyntax::kString);
  syn.addFlag("-pa", "-parent", MSyntax::kString);
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerCreateLayer::isUndoable() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::doIt(const MArgList& argList)
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::doIt\n");
  try
  {
    MArgDatabase args = makeDatabase(argList);

    AL_MAYA_COMMAND_HELP(args, g_helpText);

    if(args.isFlagSet("-o"))
    {
      // extract remaining args
      args.getFlagArgument("-o", 0, m_filePath);
    }

    if(args.isFlagSet("-pa"))
    {
      // extract remaining args
      args.getFlagArgument("-pa", 0, m_parentLayerName);
    }

    // Determine the Parent node
    MObject layer = getSelectedNode(args, nodes::Layer::kTypeId);

    m_shape = getShapeNode(args);

    if(!m_shape)
    {
      throw MS::kFailure;
    }

    if(m_parentLayerName.length() > 0)
    {
      UsdStageRefPtr stage = m_shape->usdStage();

      if(!stage)
      {
        MGlobal::displayError("no valid stage found on the proxy shape");
        throw MS::kFailure;
      }

      m_rootLayer = SdfLayer::Find(convert(m_parentLayerName));
      if(!m_rootLayer)
      {
        std::stringstream ss("LayerCreateLayer:", std::ios_base::app | std::ios_base::out);
        ss << "Unable to find the parent layer within USD, with identifier '" << m_parentLayerName << "'" << std::endl;
        MGlobal::displayError(ss.str().c_str());
        throw MS::kFailure;
      }

      m_parentLayer = m_shape->findLayer(m_rootLayer);
      if(!m_parentLayer)
      {
        std::stringstream ss("LayerCreateLayer:", std::ios_base::app | std::ios_base::out);
        ss << "Unable to find the parent layer within Maya, with identifier '" << m_parentLayerName << "'" << std::endl;
        MGlobal::displayError(ss.str().c_str());
        throw MS::kFailure;
      }
    }
    else
    if(layer == MObject::kNullObj)
    {
      UsdStageRefPtr stage = m_shape->usdStage();
      if(!stage)
      {
        MGlobal::displayError("no valid stage found on the proxy shape");
        throw MS::kFailure;
      }
      m_rootLayer = stage->GetRootLayer();
      m_parentLayer = m_shape->findLayer(m_rootLayer);
      if(!m_parentLayer)
      {
        MGlobal::displayError("LayerCreateLayer:Catastrophic failure when trying to retrieve the RootLayer");
        throw MS::kFailure;
      }
    }
    else
    {
      MFnDependencyNode fn(layer);
      m_parentLayer = (nodes::Layer*)fn.userNode();
      m_rootLayer = m_parentLayer->getHandle();
    }
  }
  catch(const MStatus& status)
  {
    return status;
  }

  if(!m_shape)
  {
    MGlobal::displayError(MString("LayerCreateLayer: Invalid shape during Layer creation"));
    return MS::kFailure;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::undoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::undoIt\n");

  // first let's go remove the newly created layer handle from the root layer we added it to
  LAYER_HANDLE_CHECK(m_newLayer->getHandle());
  SdfLayerHandle handle = m_newLayer->getHandle();

  // delete the Layer node
  MDGModifier mod;
  mod.deleteNode(m_layerNode);
  mod.doIt();

  // lots more to do here!
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::redoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::redoIt\n");
  UsdStageRefPtr childStage;

  SdfLayerRefPtr handle = SdfLayer::FindOrOpen(convert(m_filePath));

  if(!handle)
  {
    MGlobal::displayError(MString("LayerCreateLayer:unable to open layer \"") + m_filePath + "\"");
    return MS::kFailure;
  }

  MSelectionList sl;
  MString mayaLayerNodeName = AL::usdmaya::nodes::Layer::toMayaNodeName(handle->GetDisplayName());

  MStatus status = sl.add(mayaLayerNodeName);

  uint32_t selectionLength =  sl.length(&status);

  MObject selectedLayer = MObject::kNullObj;
  if(selectionLength)
  {
    sl.getDependNode(0, selectedLayer);
    if(selectedLayer.apiType() == MFn::kPluginDependNode)
    {
      sl.getDependNode(0, selectedLayer);
      MGlobal::displayInfo("LayerCreateLayer: There exists a maya layer for this node already. Not creating a new layer.");
      return MS::kSuccess;
    }
  }

  // construct the new layer node
  MFnDependencyNode fn;
  m_layerNode = fn.create(nodes::Layer::kTypeId);
  fn.setName(nodes::Layer::toMayaNodeName(handle->GetDisplayName()));
  m_newLayer = (nodes::Layer*)fn.userNode();

  m_newLayer->init(m_shape, handle);
  m_parentLayer->addChildLayer(m_newLayer);

  std::stringstream ss("LayerCreateLayer:", std::ios_base::app | std::ios_base::out);
  MGlobal::displayInfo(ss.str().c_str());
  return MS::kSuccess;
}



//----------------------------------------------------------------------------------------------------------------------
// LayerCreateSubLayer
//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(LayerCreateSubLayer, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerCreateSubLayer::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.addFlag("-o", "-open", MSyntax::kString);
  syn.addFlag("-c", "-create", MSyntax::kString);
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerCreateSubLayer::isUndoable() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateSubLayer::doIt(const MArgList& argList)
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateSubLayer::doIt\n");
  try
  {
    MArgDatabase args = makeDatabase(argList);
    AL_MAYA_COMMAND_HELP(args, g_helpText);

    if(args.isFlagSet("-c"))
    {
      // extract remaining args
      args.getFlagArgument("-c", 0, m_filePath);
      m_isOpening = false;
    }
    else
    if(args.isFlagSet("-o"))
    {
      // extract remaining args
      args.getFlagArgument("-o", 0, m_filePath);
      m_isOpening = true;
    }

    MObject layer = getSelectedNode(args, nodes::Layer::kTypeId);
    if(layer == MObject::kNullObj)
    {
      nodes::ProxyShape* proxyNode = getShapeNode(args);
      UsdStageRefPtr stage = proxyNode->usdStage();
      if(!stage)
      {
        MGlobal::displayError("no valid stage found on the proxy shape");
        throw MS::kFailure;
      }
      m_rootLayer = stage->GetEditTarget().GetLayer();
      m_parentLayer = proxyNode->findLayer(m_rootLayer);
      if(!m_parentLayer)
      {
        MGlobal::displayError("Catastrophic failure when trying to retrieve the edit target");
        throw MS::kFailure;
      }
    }
    else
    {
      MFnDependencyNode fn(layer);
      m_parentLayer = (nodes::Layer*)fn.userNode();
      m_rootLayer = m_parentLayer->getHandle();
    }
  }
  catch(const MStatus& status)
  {
    return status;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateSubLayer::undoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateSubLayer::undoIt\n");
  m_parentLayer->removeSubLayer(m_newLayer);

  // first let's go remove the newly created layer handle from the root layer we added it to
  LAYER_HANDLE_CHECK(m_newLayer->getHandle());
  SdfLayerHandle handle = m_newLayer->getHandle();
  SdfSubLayerProxy proxy = m_rootLayer->GetSubLayerPaths();

  // remove the layer, and save the original layer to reflect the changes.
  proxy.Remove(handle->GetIdentifier());
  m_rootLayer->Save();

  // delete the Layer node
  MDGModifier mod;
  mod.deleteNode(m_layerNode);
  mod.doIt();

  // lots more to do here!
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateSubLayer::redoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateSubLayer::redoIt\n");
  UsdStageRefPtr childStage;
  SdfLayerHandle handle;
  if(!m_isOpening)
  {
    childStage = UsdStage::CreateNew(convert(m_filePath));
    if(!childStage)
    {
      handle = childStage->GetRootLayer();
      if(!handle)
      {
        return MS::kFailure;
      }
      else
      {
        handle->Save();
      }
    }
  }
  else
  {
    handle = SdfLayer::FindOrOpen(convert(m_filePath));
    if(!handle)
    {
      MGlobal::displayError(MString("unable to open layer \"") + m_filePath + "\"");
      return MS::kFailure;
    }
  }
  // construct the new layer node
  MFnDependencyNode fn;
  m_layerNode = fn.create(nodes::Layer::kTypeId);
  m_newLayer = (nodes::Layer*)fn.userNode();
  m_rootLayer->GetSubLayerPaths().push_back(handle->GetIdentifier());
  m_rootLayer->Save();
  m_newLayer->init(m_shape, handle);
  m_parentLayer->addSubLayer(m_newLayer);
  fn.setName(nodes::Layer::toMayaNodeName(handle->GetDisplayName()));
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// LayerCurrentEditTarget
//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(LayerCurrentEditTarget, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerCurrentEditTarget::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.enableQuery(true);
  syn.addFlag("-l", "-layer", MSyntax::kString);
  syn.addFlag("-sp", "-sourcePath",   MSyntax::kString);
  syn.addFlag("-tp", "-targetPath", MSyntax::kString);
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  syn.addFlag("-fdn", "-findByDisplayName", MSyntax::kNoArg);
  syn.addFlag("-fid", "-findByIdentifier", MSyntax::kNoArg);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerCurrentEditTarget::isUndoable() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
PcpNodeRef LayerCurrentEditTarget::determineEditTargetMapping(UsdStageRefPtr stage,
                                                              const MArgDatabase& args,
                                                              SdfLayerHandle editTargetLayer) const
{
  if(editTargetLayer.IsInvalid())
  {
    return PcpNodeRef();
  }

  if(args.isFlagSet("-sp") && args.isFlagSet("-tp"))
  {
    MString targetPath;
    MString sourcePath;
    args.getFlagArgument("-tp", 0, targetPath);
    args.getFlagArgument("-sp", 0, sourcePath);

    UsdPrim parentPrim = stage->GetPrimAtPath(SdfPath(targetPath.asChar()));
    if( ! parentPrim.IsValid() )
    {
      std::stringstream ss("LayerCurrentEditTarget:", std::ios_base::app | std::ios_base::out);
      ss << "Couldn't find the parent prim at path '" << targetPath.asChar() << "'" << std::endl;
      MGlobal::displayWarning(ss.str().c_str());
      return PcpNodeRef();
    }

    SdfReferenceListOp referenceListOp;
    if(parentPrim.GetMetadata<SdfReferenceListOp>(TfToken("references"), &referenceListOp))
    {
      // TODO: I doubt this is the correct way to get current references. The API for UsdPrim.GetReferences() isn't sufficient!
      // TODO: Spiff recommends getting the references a different way,
      // TODO: as mentioned here https://groups.google.com/forum/#!topic/usd-interest/o6jK0tVw2eU
      const SdfListOp<SdfReference>::ItemVector& addedItems = referenceListOp.GetAddedItems();

      size_t referenceListSize = addedItems.size();
      for(size_t i = 0; i < referenceListSize; ++i)
      {
        SdfLayerHandle referencedLayer = SdfLayer::Find(addedItems[i].GetAssetPath());

        // Is the referenced layer referring to the layer we selected?
        if(referencedLayer == editTargetLayer)
        {
          PcpNodeRef::child_const_range childRange = parentPrim.GetPrimIndex().GetRootNode().GetChildrenRange();
          PcpNodeRef::child_const_iterator currentChildSpec = childRange.first;
          PcpNodeRef::child_const_iterator end = childRange.second;
          while(currentChildSpec != end)
          {
            if(currentChildSpec->GetParentNode().GetPath() == SdfPath(targetPath.asChar())
                && currentChildSpec->GetPath() == SdfPath(sourcePath.asChar()))
            {
              return *currentChildSpec;
            }
            ++currentChildSpec;
          }
        }
      }

      MGlobal::displayWarning("LayerCurrentEditTarget: Couldn't find the PcpNodeRef to initialise the MappingFunction for the EditTarget");
    }
  }
  else
  {
    MGlobal::displayInfo("LayerCurrentEditTarget: Default MappingFunction for EditTarget will be used since sp(Source Prim) and tp(Target Prim) flags were not set");
  }

  return PcpNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::doIt(const MArgList& argList)
{
  try
  {
    MArgDatabase args = makeDatabase(argList);
    AL_MAYA_COMMAND_HELP(args, g_helpText);
    if(args.isQuery())
    {
      isQuery = true;
      stage = getShapeNodeStage(args);
      if(stage)
      {
        const UsdEditTarget& editTarget = stage->GetEditTarget();
        previous = editTarget;
      }
      else
      {
        MGlobal::displayError("LayerCurrentEditTarget: no loaded stage found on proxy node");
        return MS::kFailure;
      }
    }
    else
    {
      // Setup the function pointers which will be used to find the wanted layer
      if(args.isFlagSet("-fid"))
      {
        // Use the Identifier when looking for the correct layer. Used for anonymous layers
        getLayerId = [](SdfLayerHandle layer) {  return layer->GetIdentifier(); };
      }
      else if(args.isFlagSet("-fdn"))
      {
        // Use the DisplayName when looking for the correct layer
        getLayerId = [](SdfLayerHandle layer) { return layer->GetDisplayName(); };
      }
      else
      {
        // Default to DisplayName if not specified for backwards compatibility
        getLayerId = [](SdfLayerHandle layer) { return layer->GetDisplayName(); };
      }

      isQuery = false;
      MObject selectedLayer = MObject::kNullObj;
      // if the layer has been manually specified
      MString layerName;
      if(args.isFlagSet("-l") && args.getFlagArgument("-l", 0, layerName))
      {
        MString mayaLayerName = AL::usdmaya::nodes::Layer::toMayaNodeName(convert(layerName));

        MSelectionList sl;
        MStatus status = sl.add(mayaLayerName);
        uint32_t selectionLength =  sl.length(&status);

        if(selectionLength)
        {
          sl.getDependNode(0, selectedLayer);
          if(selectedLayer.apiType() == MFn::kPluginDependNode)
          {
            MFnDependencyNode fnDep(selectedLayer);
            if(fnDep.typeId() != nodes::Layer::kTypeId)
            {
              selectedLayer = MObject::kNullObj;
            }
          }
          else
          {
            selectedLayer = MObject::kNullObj;
          }
        }
      }

      if(selectedLayer == MObject::kNullObj)
      {
        selectedLayer = getSelectedNode(args, nodes::Layer::kTypeId);
      }

      SdfLayerHandle foundLayer = nullptr;
      // check to see if a layer is selected.
      if(selectedLayer != MObject::kNullObj)
      {
        MFnDependencyNode fnLayer(selectedLayer);
        m_usdLayer = (nodes::Layer*)fnLayer.userNode();
        LAYER_HANDLE_CHECK(m_usdLayer->getHandle());
        foundLayer = m_usdLayer->getHandle();
        previouslyAnEditTarget = m_usdLayer->hasBeenTheEditTarget();

        MObject proxyNode = MObject::kNullObj;
        MObject temp = selectedLayer;
        while(proxyNode == MObject::kNullObj)
        {
          MPlug parentLayerPlug(temp, nodes::Layer::parentLayer());
          MPlug parentShapePlug(temp, nodes::Layer::proxyShape());

          // yay! we've found the proxy shape
          if(parentShapePlug.isConnected())
          {
            MPlugArray plugs;
            parentShapePlug.connectedTo(plugs, true, true);
            if(plugs.length())
            {
              proxyNode = plugs[0].node();
            }
            break;
          }
          if(parentLayerPlug.isConnected())
          {
            MPlugArray plugs;
            parentLayerPlug.connectedTo(plugs, true, true);
            if(plugs.length())
            {
              temp = plugs[0].node();
            }
            else
            {
              MGlobal::displayError("upstream proxy shape could not be found");
              return MS::kFailure;
            }
          }
        }

        if(proxyNode == MObject::kNullObj)
        {
          MGlobal::displayError("upstream proxy shape could not be found");
          return MS::kFailure;
        }
        MFnDependencyNode fnDep(proxyNode);
        if(proxyNode.hasFn(MFn::kPluginShape))
        {
          if(fnDep.typeId() == nodes::ProxyShape::kTypeId)
          {
            nodes::ProxyShape* usdProxy = (nodes::ProxyShape*)fnDep.userNode();
            stage = usdProxy->usdStage();
          }
        }
      }
      else
      {
        stage = getShapeNodeStage(args);
      }

      if(stage)
      {
        previous = stage->GetEditTarget();
        isQuery = false;
        std::string layerName2;
        if(foundLayer)
        {

          PcpNodeRef mappingNode = determineEditTargetMapping(stage, args, foundLayer);
          if(mappingNode)
          {
            next = UsdEditTarget(foundLayer, mappingNode);
          }
          else
          {
            next = UsdEditTarget(foundLayer);
          }
          layerName2 = getLayerId(next.GetLayer());
        }
        else
        if(args.isFlagSet("-l"))
        {
          MString layerName;
          args.getFlagArgument("-l", 0, layerName);
          layerName2 = convert(layerName);
          SdfLayerHandleVector layers = stage->GetUsedLayers();
          for(auto it = layers.begin(); it != layers.end(); ++it)
          {

            SdfLayerHandle handle = *it;

            if(layerName2 == getLayerId(handle))
            {
              PcpNodeRef mappingNode = determineEditTargetMapping(stage, args, handle);
              if(mappingNode)
              {
                next = UsdEditTarget(handle, mappingNode);
              }
              else
              {
                next = UsdEditTarget(handle);
              }
              break;
            }
          }
        }
        else
        {
          MGlobal::displayError("No layer specified");
          return MS::kFailure;
        }

        if(!next.IsValid())
        {
          // if we failed to find the layer in the list of used layers, just check to see whether we are actually able to
          // edit said layer.
          SdfLayerHandleVector layers = stage->GetUsedLayers();
          for(auto it = layers.begin(); it != layers.end(); ++it)
          {
            SdfLayerHandle handle = *it;
            if(layerName2 == getLayerId(handle))
            {
              MGlobal::displayError("LayerCurrentEditTarget: Unable to set the edit target, the specified layer cannot be edited");
              return MS::kFailure;
            }
          }
          MGlobal::displayError(MString("LayerCurrentEditTarget: no layer found on proxy node that matches the name \"") + convert(layerName2) + "\"");
          return MS::kFailure;
        }
      }
      else
      {
        MGlobal::displayError("LayerCurrentEditTarget: no loaded stage found on proxy node");
        return MS::kFailure;
      }
    }
  }
  catch(const MStatus& status)
  {
    MGlobal::displayError("LayerCurrentEditTarget: no proxy node found");
    return status;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::redoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::redoIt\n");
  if(!isQuery)
  {
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::redoIt setting target: %s\n", next.GetLayer()->GetDisplayName().c_str());
    stage->SetEditTarget(next);

    if(m_usdLayer)
    {
      m_usdLayer->setHasBeenTheEditTarget(true);
    }
  }
  else
  {
    // there are cases now where the layer may not have a name, so we need to hunt for the layer.
    // This is going to be safer in the long run anyway :)
    MItDependencyNodes it(MFn::kPluginDependNode);
    for(; !it.isDone(); it.next())
    {
      MFnDependencyNode fn(it.item());
      if(fn.typeId() == nodes::Layer::kTypeId)
      {
        nodes::Layer* layer = (nodes::Layer*)fn.userNode();
        if(layer)
        {
          if(previous.GetLayer() == layer->getHandle())
          {
            setResult(fn.name());
            return MS::kSuccess;
          }
        }
      }
    }

    setResult(convert(previous.GetLayer()->GetDisplayName()));
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::undoIt()
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::undoIt\n");
  if(!isQuery)
  {
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::undoIt setting target: %s\n", previous.GetLayer()->GetDisplayName().c_str());
    stage->SetEditTarget(previous);
    m_usdLayer->setHasBeenTheEditTarget(previouslyAnEditTarget);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// LayerSave
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerSave, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerSave::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.addFlag("-l", "-layer", MSyntax::kString);
  syn.addFlag("-f", "-filename", MSyntax::kString);
  syn.addFlag("-s", "-string", MSyntax::kNoArg);
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerSave::isUndoable() const
{
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSave::doIt(const MArgList& argList)
{
  try
  {
    MArgDatabase args = makeDatabase(argList);
    AL_MAYA_COMMAND_HELP(args, g_helpText);


    MObject layerNode = MObject::kNullObj;
    // if the layer has been manually specified
    if(args.isFlagSet("-l"))
    {
      MString layerName;
      if(args.getFlagArgument("-l", 0, layerName))
      {
        MSelectionList sl;
        sl.add(layerName);
        if(sl.length())
        {
          sl.getDependNode(0, layerNode);
          if(layerNode.apiType() == MFn::kPluginDependNode)
          {
            MFnDependencyNode fnDep(layerNode);
            if(fnDep.typeId() != nodes::Layer::kTypeId)
            {
              layerNode = MObject::kNullObj;
            }
          }
          else
            layerNode = MObject::kNullObj;
        }
      }
    }

    if(layerNode == MObject::kNullObj)
    {
      layerNode = getSelectedNode(args, nodes::Layer::kTypeId);
    }

    if(layerNode == MObject::kNullObj)
    {
      MGlobal::displayError("LayerSave: you need to specify an Layer node that you wish to save");
      throw MS::kFailure;
    }

    MFnDependencyNode fn(layerNode);
    nodes::Layer* layer = (nodes::Layer*)fn.userNode();
    LAYER_HANDLE_CHECK(layer->getHandle());
    SdfLayerHandle handle = layer->getHandle();
    if(handle)
    {
      bool flatten = args.isFlagSet("-fl");
      if(flatten)
      {
        if(!args.isFlagSet("-f") && !args.isFlagSet("-s"))
        {
          MGlobal::displayError("LayerSave: when using -flatten/-fl, you must specify the filename");
          throw MS::kFailure;
        }

        // grab the path to the layer.
        const std::string filename = handle->GetRealPath();
        std::string outfilepath;
        if(args.isFlagSet("-f"))
        {
          MString temp;
          args.getFlagArgument("-f", 0, temp);
          outfilepath = convert(temp);
        }

        // make sure the user is not going to annihilate their own work.
        // I should probably put more checks in here? Or just remove this check and
        // assume user error is not a thing?
        if(outfilepath == filename)
        {
          MGlobal::displayError("LayerSave: nice try, but no, I'm not going to let you overwrite the layer with a flattened version."
                                "\nthat would seem like a very bad idea to me.");
          throw MS::kFailure;
        }
      }
      else
      {
        bool exportingToString = args.isFlagSet("-s");
        if(exportingToString)
        {
          // just set the text string as the result of the command
          std::string temp;
          handle->ExportToString(&temp);
          setResult(convert(temp));
        }
        else
        {
          // if exporting to a file
          if(args.isFlagSet("-f"))
          {
            MString temp;
            args.getFlagArgument("-f", 0, temp);
            const std::string filename = convert(temp);
            bool result = handle->Export(filename);
            setResult(result);
            if(!result) MGlobal::displayError("LayerSave: could not export layer");
          }
          else
          {
            bool result = handle->Save();
            setResult(result);
            if(!result) MGlobal::displayError("LayerSave: could not save layer");
          }
        }
      }
    }
    else
    {
      MGlobal::displayError("LayerSave: No valid layer handle found");
      throw MS::kFailure;
    }
  }
  catch(const MStatus& status)
  {
    return status;
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set whether the layer is currently muted
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerSetMuted, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerSetMuted::createSyntax()
{
  MSyntax syn = setUpCommonSyntax();
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  syn.addFlag("-m", "-muted", MSyntax::kBoolean);
  return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerSetMuted::isUndoable() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::doIt(const MArgList& argList)
{
  try
  {
    MArgDatabase args = makeDatabase(argList);
    AL_MAYA_COMMAND_HELP(args, g_helpText);

    MObject layerNode = getSelectedNode(args, nodes::Layer::kTypeId);
    if(layerNode == MObject::kNullObj)
    {
      MGlobal::displayError("LayerSetMuted: you need to specify an Layer node that you wish to mute/unmute");
      throw MS::kFailure;
    }

    if(!args.isFlagSet("-m"))
    {
      MGlobal::displayError("LayerSetMuted: please tell me whether you want to mute or unmute via the -m <bool> flag");
      throw MS::kFailure;
    }

    MFnDependencyNode fn(layerNode);
    nodes::Layer* layer = (nodes::Layer*)fn.userNode();
    LAYER_HANDLE_CHECK(layer->getHandle());
    m_layer = layer->getHandle();
    if(!m_layer)
    {
      MGlobal::displayError("LayerSetMuted: no valid USD layer found on the node");
      throw MS::kFailure;
    }

    args.getFlagArgument("-m", 0, m_muted);
  }
  catch(const MStatus& status)
  {
    return status;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::undoIt()
{
  if(m_layer)
    m_layer->SetMuted(m_muted);
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::redoIt()
{
  if(m_layer)
    m_layer->SetMuted(m_muted);
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray buildLayerList(const MString&)
{
  MStringArray result;
  MItDependencyNodes it(MFn::kPluginDependNode);
  for(; !it.isDone(); it.next())
  {
    MFnDependencyNode fn(it.item());
    if(fn.typeId() == nodes::Layer::kTypeId)
    {
      result.append(fn.name());
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void constructLayerCommandGuis()
{
  {
    maya::CommandGuiHelper saveLayer("AL_usdmaya_LayerSave", "Save Layer", "Save Layer", "USD/Layers/Save Layer", false);
    saveLayer.addListOption("l", "Layer to Save", (AL::maya::GenerateListFn)buildLayerList);
    saveLayer.addFilePathOption("f", "USD File Path", maya::CommandGuiHelper::kSave, "USDA files (*.usda) (*.usda);;USDC files (*.usdc) (*.usdc);;Alembic Files (*.abc) (*.abc);;All Files (*) (*)", maya::CommandGuiHelper::kStringMustHaveValue);
  }

  {
    maya::CommandGuiHelper createSubLayer("AL_usdmaya_LayerCreateSubLayer", "Create Sub Layer on current layer", "Create", "USD/Layers/Create Sub Layer", false);
    createSubLayer.addFilePathOption("create", "Create New Layer", maya::CommandGuiHelper::kSave, "USD files (*.usd*) (*.usd*);; Alembic Files (*.abc) (*.abc);;All Files (*) (*)", maya::CommandGuiHelper::kStringOptional);
    createSubLayer.addFilePathOption("open", "Open Existing Layer", maya::CommandGuiHelper::kLoad, "USD files (*.usd*) (*.usd*);; Alembic Files (*.abc) (*.abc);;All Files (*) (*)", maya::CommandGuiHelper::kStringOptional);
  }

  {
    maya::CommandGuiHelper createLayer("AL_usdmaya_LayerCreateLayer", "Create Layer on current layer", "Create", "USD/Layers/Create Sub Layer", false);
    createLayer.addFilePathOption("open", "Find or Open Existing Layer", maya::CommandGuiHelper::kLoad, "USD files (*.usd*) (*.usd*);; Alembic Files (*.abc) (*.abc);;All Files (*) (*)", maya::CommandGuiHelper::kStringOptional);
  }

  {
    maya::CommandGuiHelper setEditTarget("AL_usdmaya_LayerCurrentEditTarget", "Set Current Edit Target", "Set", "USD/Layers/Set Current Edit Target", false);
    setEditTarget.addListOption("l", "USD Layer", (AL::maya::GenerateListFn)buildLayerList);
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Documentation strings.
//----------------------------------------------------------------------------------------------------------------------
const char* const LayerCreateLayer::g_helpText = R"(
LayerCreateLayer Overview:

  This command provides a way to create new layers in Maya. The Layer identifier passed into the -o will attempt to find the layer, 
  and if it doesn't exist then it is created. If a layer is created, it will create a AL::usdmaya::nodes::Layer which will contain a SdfLayerRefPtr 
  to the layer opened with -o. This layer can also be parented under an existing layer by passing in the identifier into -pa.
   
  This command is currently used in our pipeline to create layers on the fly. These layers may then be targeted by an EditTarget for edits
  and these edits are saved into the maya scene file. 

  If the -pa(parent) is the identifier of the layer in USD. If a corresponding Sdf.Layer cannot 
  be found the command will return a failure, once the Sdf.Layer is found it will try find the reciprocal layer in Maya,
  if this layer can't be found the command will return a failure.

If no identifier is passed, the stage's root layer is used as the parent.

  Examples:
    To create a layer in maya and implicitly parent it to Maya's root layer representation
      AL_usdmaya_LayerCreateLayer -o "path/to/layer.usda" -p "ProxyShape1"

    To create a layer and parent it to a layer existing
      AL_usdmaya_LayerCreateLayer -o "path/to/layer.usda" -pa "exisiting/layers/identifier.usda" -p "ProxyShape1"
)";

const char* const LayerGetLayers::g_helpText = R"(
LayerGetLayers Overview:

  This command provides a way to query the various layers on an ProxyShape.
  There are 4 main types of layer that you can query:

    1. Muted layers: These layers are effectively disabled (muted in USD speak).
    2. Used layers: These are the current layers in use by the proxy shape node. This can
       either be queried as a flattened list, or as a hierarchy.
    3. Session Layers: This is the highest level layer, used to store changes made for
       your session, e.g. visibility changes, wireframe display mode, etc.
    4. Layer Stack: This is a stack of layers that can be set as edit targets. This implicitly
       includes the session layer for this current session, but you can choose to filter that
       out.

  An ProxyShape node must either be selected when running this command, or it must be
  specified as the final argument to this command.

  By default, the command will return the USD layer display names (e.g. "myLayer.udsa"). If you
  wish to return the names of the maya nodes that are currently mirroring them, add the flag
  "-mayaNames" to any of the following examples:

Examples:

  To query the muted layers:

      LayerGetLayers -muted "ProxyShape1";

  To query the used layers as a flattened list:

      LayerGetLayers -used "ProxyShape1";

  To query the used layers as a hierarchy:

      LayerGetLayers -hierarchy "ProxyShape1";

  To query the usd layer stack (without the session layer):

      LayerGetLayers -stack "ProxyShape1";

  To query the usd layer stack (with the session layer):

      LayerGetLayers -stack -sessionLayer "ProxyShape1";

  To query the usd session layer on its own:

      LayerGetLayers -sessionLayer "ProxyShape1";

  To query the usd root layer on its own:

      LayerGetLayers -rootLayer "ProxyShape1";
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerCreateSubLayer::g_helpText = R"(
LayerCreateSubLayer Overview:

  Given a USD layer, this command will allow you to create a new sub-layer on that layer. If you
  specify an ProxyShape, either by selecting it, or by specifying its name as the last
  argument to this command, then the sub-layer will be created to that proxy nodes' current edit
  target.

  Alternatively, if you select a USD layer (or specify the maya node as the last param to this command),
  then the sublayer will be added under the specified layer.

  To query or set the current edit target, use the LayerCurrentEditTarget command (for example,
  you might want to set your newly created sub layer to be the edit target, or you might want to
  query/control where the sub-layer will be created).

  You will always need to specify a filepath to the USD file for your sublayer. You can do this either
  with the -create/-c option (which will create a new usda file for you layer), or via the -open/-o
  flag to open an existing layer. If -create is used, and the file already exists, an error will
  be generated. If -open/-o is specified, and the file does not exist, an error will be generated.

  This command is undoable.

Examples:

  To create a new sub-layer on the current edit target of a ProxyShape:

    LayerCreateSubLayer -c "/my/file/path.usda"  "ProxyShape1"; // create new usd file
    LayerCreateSubLayer -o "/my/file/path.usda"  "ProxyShape1"; // open existing usd file

  To create a new sub-layer on the a specified Layer node:

    LayerCreateSubLayer -c "/my/file/path.usda"  "Layer1"; // create new usd file
    LayerCreateSubLayer -o "/my/file/path.usda"  "Layer1"; // open existing usd file

Possible Problems:

  Currently no checking is performed to see if there are circular references. I have no idea what
  would happen if you were to attempt to add a parent layer as a sub layer of one of its children.
  Bad things I'd imagine!

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerCurrentEditTarget::g_helpText = R"(
LayerCurrentEditTarget Overview:

  Within the USD stage contained within an ProxyShape node, a single layer may be set as the
  edit target at any given time. Any changes made to the contents of a USD proxy node, will end up
  being stored within that layer.

  To determine the current edit target for the currently selected ProxyShape, you can simply
  execute this command:

    LayerCurrentEditTarget -q;

  To determine the edit target on a specific proxy shape node, you can append the name of the shape
  to the end of the command:

    LayerCurrentEditTarget -q "ProxyShape1";

  To set the edit target on a proxy node, there are a few approaches:

  1. Select a ProxyShape, and specify the name of the layer to set via the "-layer" flag:

     LayerCurrentEditTarget -l "Layer1";

  2. Specify the name of the layer via the "-layer" flag, and specify the ProxyShape name:

     LayerCurrentEditTarget -l "Layer1" "ProxyShape1";

  3. Specify name of the layer as well as specifying parameters to the EditTargets mapping function
     LayerCurrentEditTarget -tp "/shot_zda01_020/environment" -sp "/ShotSetRoot" -l "Layer1" "ProxyShape1"


  4. Select the Layer in maya, and run the command:

     LayerCurrentEditTarget;

  5. Specify the layer name as an identifier:
     LayerCurrentEditTarget -l "anon:0x136d9050" -fid -proxy "ProxyShape1"


  There are some caveats here though. If no TargetPath and SourcePath prim paths are specified, 
  USD will only allow you to set an edit target into what is known as the current layer stack. 
  These layers can be determined using the following command:

     LayerGetLayers -stack "ProxyShape1";

  These usually include the current root layer (LayerGetLayers -rootLayer "ProxyShape1"),
  the current session layer ((LayerGetLayers -sessionLayer "ProxyShape1"), and any sub
  layers of those two layers. Attempting to set an edit target on a layer that is not in the layer
  stack and without providing the TargetPath or SourcePath is an error.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerSave::g_helpText = R"(
LayerSave Overview:

  This command allows you to export/save a single layer to a file. In the simplest case, if you select
  an Layer node, you can simply execute:

     LayerSave;

  This will save that layer to disk (using the existing file path set on the node). Alternatively you
  can also specify the layer name to save, e.g.

     LayerSave "myscene_root_usda";

  If you wish to export that layer and return it as a text string, use the -string/-s flag. The following
  command will return the usd file contents as a string.

     LayerSave -s "myscene_root_usda";

  If you wish to export that layer as a new file, you can also specify the filepath with the -f/-filename
  flag, e.g.

     LayerSave -f "/scratch/stuff/newlayer.usda" "myscene_root_usda";

  In addition, you are also able to flatten a given layer using the -flatten option. When using this
  option, the specified layer will be written out as a new file, and that file will contain ALL of the
  data from that layers child layers and sublayers. This can result in some fairly large files!
  Note: when using the -flatten option, you must specify the -s or -f flags (to write to a string,
  or export as a file)

     LayerSave -flatten -f "/scratch/stuff/phatlayer.usda" "myscene_root_usda";

  or to return a string

     LayerSave -flatten -s "myscene_root_usda";

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerSetMuted::g_helpText = R"(
LayerSetMuted Overview:

  This command allows you to mute or unmute a specified layer. If you have a layer selected:

     LayerSetMuted -m true;  //< mutes the currently selected layer
     LayerSetMuted -m false;  //< unmutes the currently selected layer

  You can also specify the layer if you wish:

     LayerSetMuted -m true "Layer1";  //< mutes the layer 'Layer1'
     LayerSetMuted -m false "Layer1";  //< unmutes the layer 'Layer1'

  This command is undoable, but it will probably crash right now.
)";

//----------------------------------------------------------------------------------------------------------------------
} // cmds
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
