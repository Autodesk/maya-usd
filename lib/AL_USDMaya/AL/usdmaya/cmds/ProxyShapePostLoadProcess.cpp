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
#include "AL/maya/CodeTimings.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageData.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/cmds/LayerCommands.h"
#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"

#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include "maya/MArgList.h"
#include "maya/MDagPath.h"
#include "maya/MDagModifier.h"
#include "maya/MDGModifier.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnCamera.h"
#include "maya/MFnTransform.h"
#include "maya/MGlobal.h"
#include "maya/MPlug.h"
#include "maya/MPlugArray.h"
#include "maya/MSelectionList.h"
#include "maya/MString.h"
#include "maya/MSyntax.h"

#include <pxr/base/tf/type.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/variantSets.h>

#include <map>
#include <string>

namespace AL {
namespace usdmaya {
namespace cmds {
namespace {

struct CompareLayerHandle
{
  bool operator () (const SdfLayerHandle& a, const SdfLayerHandle& b) const
  {
    return a->GetDisplayName() < b->GetDisplayName();
  }
};

typedef std::set<SdfLayerHandle, CompareLayerHandle> LayerSet;
typedef std::map<SdfLayerHandle, LayerSet, CompareLayerHandle > LayerMap;
typedef std::map<SdfLayerHandle, MObject, CompareLayerHandle > LayerToObjectMap;

//----------------------------------------------------------------------------------------------------------------------
SdfLayerHandle findLayer(const SdfLayerHandleVector& layers, const std::string& name)
{
  for(auto it = layers.begin(); it != layers.end(); ++it)
  {
    if(name == (*it)->GetDisplayName() ||
       name == (*it)->GetIdentifier())
    {
      return *it;
    }
  }
  return SdfLayerHandle();
}

//----------------------------------------------------------------------------------------------------------------------
void buildTree(const SdfLayerHandle& layer, LayerMap& layerMap, const SdfLayerHandleVector& layers)
{
  auto iter = layerMap.find(layer);
  if(iter == layerMap.end())
  {
    LayerSet kids;
    std::set<std::string> refs = layer->GetExternalReferences();


    for(auto it = refs.begin(); it != refs.end(); ++it)
    {
      SdfLayerHandle childHandle = findLayer(layers, *it);

      if(childHandle)
      {
        kids.insert(childHandle);
        buildTree(childHandle, layerMap, layers);
      }
    }
    layerMap.insert(std::make_pair(layer, kids));
  }
}

//----------------------------------------------------------------------------------------------------------------------
struct ImportCallback
{
  enum ScriptType : uint32_t
  {
    kMel,
    kPython
  };

  void setCallbackType(TfToken scriptType)
  {
    if(scriptType == "mel")
    {
      type = kMel;
    }
    else
    if(scriptType == "py")
    {
      type = kPython;
    }
  }
  std::string name;
  VtDictionary params;
  ScriptType type;
};

//----------------------------------------------------------------------------------------------------------------------
void huntForNativeNodes(
    const MDagPath& proxyTransformPath,
    std::vector<UsdPrim>& schemaPrims,
    std::vector<ImportCallback>& postCallBacks,
    UsdStageRefPtr stage,
    fileio::translators::TranslatorManufacture& manufacture)
{
  fileio::SchemaPrimsUtils utils(manufacture);
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("huntForNativeNodes::huntForNativeNodes\n");
  fileio::TransformIterator it(stage, proxyTransformPath);
  for(; !it.done(); it.next())
  {
    UsdPrim prim = it.prim();
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("huntForNativeNodes: %s\n", prim.GetName().GetText());
    if(utils.isSchemaPrim(prim))
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::huntForNativeNodes found matching schema %s\n", prim.GetPath().GetText());
      schemaPrims.push_back(prim);
    }

    VtDictionary customData = prim.GetCustomData();
    VtDictionary::const_iterator postCallBacksEntry = customData.find("callbacks");
    if(postCallBacksEntry != customData.end())
    {
      //Get the list of post callbacks
      VtDictionary melCallbacks = postCallBacksEntry->second.Get<VtDictionary>();

      for(VtDictionary::const_iterator melCommand = melCallbacks.begin(), end = melCallbacks.end();
          melCommand != end;
          ++melCommand)
      {
        ImportCallback importCallback;
        importCallback.name = melCommand->first;
        importCallback.type = ImportCallback::kMel;
        importCallback.params = melCommand->second.Get<VtDictionary>();

        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::huntForNativeNodes adding post callback from %s\n", prim.GetPath().GetText());
        postCallBacks.push_back(importCallback);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MObject makeLayerNode(SdfLayerHandle layer, LayerToObjectMap& layerToObjectMap, nodes::ProxyShape* proxyShape)
{
  LAYER_HANDLE_CHECK(layer);
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapePostLoadProcess::makeLayerNode %s\n", layer->GetDisplayName().c_str());
  MFnDependencyNode fn;
  MObject layerNode = fn.create(nodes::Layer::kTypeId);


  UsdStageRefPtr stage = proxyShape->getUsdStage();
  MString layerName;

  if(!layer->IsAnonymous())
  {
    layerName = nodes::Layer::toMayaNodeName(layer->GetDisplayName());
  }
  else if(stage->GetSessionLayer() == layer)
  {
    layerName = MString("session_layer_usda");
  }
  else
  {
    layerName =  AL::usdmaya::convert(layer->GetIdentifier()); // Assuming Anonymous' layer, use identifier because it has no display name.
  }

  fn.setName(layerName);

  // construct map from sdf layer, to the maya node for that layer
  layerToObjectMap.insert(std::make_pair(layer, layerNode));

  nodes::Layer* layerObject = (nodes::Layer*)fn.userNode();
  layerObject->init(proxyShape, layer);
  return layerNode;
}

//----------------------------------------------------------------------------------------------------------------------
void buildLayerTree(MObject layerNode, const SdfLayerHandle& layer, LayerMap& layerMap, LayerToObjectMap& layersToObjects, MDGModifier& modifier, nodes::ProxyShape* proxyShape)
{
  LAYER_HANDLE_CHECK(layer);
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapePostLoadProcess::buildLayerTree %s\n", layer->GetDisplayName().c_str());
  {
    // first attempt to create any sub layers that are connected to the input layer
    SdfSubLayerProxy subLayers = layer->GetSubLayerPaths();

    if(!subLayers.empty())
    {
      MPlug subLayersPlug(layerNode, nodes::Layer::subLayers());
      subLayersPlug.setNumElements(subLayers.size());
      uint32_t currChild = 0;

      // build up nodes for each of the sub layers
      for(auto it = subLayers.begin(); it != subLayers.end(); ++it, ++currChild)
      {
        const std::string& name = *it;
        SdfLayerHandle subLayerHandle = SdfLayer::Find(name);
        if(subLayerHandle)
        {
          // construct the node
          MObject newLayer = makeLayerNode(subLayerHandle, layersToObjects, proxyShape);

          // connect to its parent layer
          MPlug plug(newLayer, nodes::Layer::parentLayer());
          if(!modifier.connect(subLayersPlug.elementByLogicalIndex(currChild), plug))
          {
            std::cerr << "Error: connection not made to sublayer " << subLayerHandle->GetDisplayName() << std::endl;
          }

          // recurse because sublayers can themselves contain sublayers.
          buildLayerTree(newLayer, subLayerHandle, layerMap, layersToObjects, modifier, proxyShape);
        }
      }
    }
  }

  auto iter = layerMap.find(layer);

  // figure out how many child layers we have on this layer
  MPlug childLayersPlug(layerNode, nodes::Layer::childLayers());
  uint32_t numElements = childLayersPlug.numElements();
  uint32_t count = numElements;
  for(auto it = iter->second.begin(); it != iter->second.end(); ++it)
  {
    if(layersToObjects.find(*it) == layersToObjects.end())
    {
      ++numElements;
    }
  }

  // resize for fun.
  childLayersPlug.setNumElements(numElements);

  // create the remaining child layers
  for(auto it = iter->second.begin(); it != iter->second.end(); ++it)
  {
    if(layersToObjects.find(*it) == layersToObjects.end())
    {
      MObject newLayer = makeLayerNode(*it, layersToObjects, proxyShape);
      MPlug plug(newLayer, nodes::Layer::parentLayer());
      modifier.connect(childLayersPlug.elementByLogicalIndex(count++), plug);

      buildLayerTree(newLayer, *it, layerMap, layersToObjects, modifier, proxyShape);
    }

  }
}

//----------------------------------------------------------------------------------------------------------------------
void constructLayers(MObject proxyNode, nodes::ProxyShape* shape, UsdStageRefPtr stage, bool include_assets = false)
{
  if(!stage)
    return;

  if(!shape)
    return;

  LayerMap layerMap;
  SdfLayerHandleVector layers = stage->GetUsedLayers();
  SdfLayerHandle previous;
  SdfLayerHandle first;
  SdfLayerHandleVector layerStack = stage->GetLayerStack(true);


  for(auto it = layerStack.begin(); it != layerStack.end(); ++it)
  {
    SdfLayerHandle handle = *it;
    if(handle)
    {
      if(!previous)
      {
        first = handle;
      }

      // first see whether previous exists in the map.
      // If it does add this handle as a child of the previous
      {
        auto iter = layerMap.find(previous);
        if(iter != layerMap.end())
        {
          iter->second.insert(handle);
        }
      }

      // now build the tree from this layer
      buildTree(handle, layerMap, layers);
      previous = handle;
    }
  }

  MDGModifier modifier;
  if(first)
  {
    LayerToObjectMap layersToObjects;
    MObject layerNode = makeLayerNode(first, layersToObjects, shape);

    // connect highest level layer to the proxy shape.
    modifier.connect(proxyNode, nodes::ProxyShape::layers(), layerNode, nodes::Layer::proxyShape());

    // now prcoess the rest of the nodes.
    buildLayerTree(layerNode, first, layerMap, layersToObjects, modifier, shape);
  }

  if(!modifier.doIt())
  {
    std::cout << "Failed to connect layers to proxy shape" << std::endl;
  }
}

} // anon

//----------------------------------------------------------------------------------------------------------------------
fileio::ImporterParams ProxyShapePostLoadProcess::m_params;

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(
    nodes::ProxyShape* ptrNode,
    const std::vector<UsdPrim>& schemaPrims,
    const MDagPath& proxyTransformPath,
    ProxyShapePostLoadProcess::MObjectToPrim& objsToCreate)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims called\n");
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims\n");
  AL_BEGIN_PROFILE_SECTION(CreateTransformChains);
  {
    objsToCreate.reserve(schemaPrims.size());
    MDagModifier modifier;
    MDGModifier modifier2;

    MPlug outStage = ptrNode->outStageDataPlug();
    MPlug outTime = ptrNode->outTimePlug();
    MFnTransform fnx(proxyTransformPath);
    fileio::SchemaPrimsUtils schemaPrimUtils(ptrNode->translatorManufacture());
    for(auto it = schemaPrims.begin(); it != schemaPrims.end(); ++it)
    {
      const UsdPrim& usdPrim = *it;
      if(usdPrim.IsValid())
      {
        SdfPath path = usdPrim.GetPath();
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims checking %s\n", path.GetText());
        MObject newpath = MObject::kNullObj;
        if(schemaPrimUtils.needsTransformParent(usdPrim))
        {
          newpath = ptrNode->makeUsdTransformChain(usdPrim, modifier, nodes::ProxyShape::kRequired, &modifier2);
        }
        objsToCreate.push_back(std::make_pair(newpath, usdPrim));
      }
      else
      {
        std::cout << "prim is invalid" << std::endl;
      }
    }

    if(!modifier.doIt())
    {
      std::cerr << "Failed to connect up attributes" << std::endl;
    }
    else
    if(!modifier2.doIt())
    {
      std::cerr << "Failed to enable pushToPrim attributes" << std::endl;
    }
  }
  AL_END_PROFILE_SECTION();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::createSchemaPrims(
    nodes::ProxyShape* proxy,
    const std::vector<UsdPrim>& objsToCreate)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createSchemaPrims\n");
  AL_BEGIN_PROFILE_SECTION(CreatePrims);
  {
    fileio::translators::TranslatorContextPtr context = proxy->context();
    fileio::translators::TranslatorManufacture& translatorManufacture = proxy->translatorManufacture();

    auto it = objsToCreate.begin();
    const auto end = objsToCreate.end();
    for(; it != end; ++it)
    {
      UsdPrim prim = *it;

      MObject object = proxy->findRequiredPath(prim.GetPath());

      fileio::translators::TranslatorRefPtr translator = translatorManufacture.get(prim.GetTypeName());
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createSchemaPrims prim=%s\n", prim.GetPath().GetText());

      //if(!context->hasEntry(prim.GetPath(), prim.GetTypeName()))
      {
        AL_BEGIN_PROFILE_SECTION(SchemaPrims);
        if(!fileio::importSchemaPrim(prim, object, 0, context, translator))
        {
          std::cerr << "Error: unable to load schema prim node: '" << prim.GetName().GetString() << "' that has type: '" << prim.GetTypeName() << "'" << std::endl;
        }
        AL_END_PROFILE_SECTION();
      }
    }
  }
  AL_END_PROFILE_SECTION();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::updateSchemaPrims(
    nodes::ProxyShape* proxy,
    const std::vector<UsdPrim>& objsToCreate)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::updateSchemaPrims\n");
  AL_BEGIN_PROFILE_SECTION(CreatePrims);
  {
    fileio::translators::TranslatorContextPtr context = proxy->context();
    fileio::translators::TranslatorManufacture& translatorManufacture = proxy->translatorManufacture();

    auto it = objsToCreate.begin();
    const auto end = objsToCreate.end();
    for(; it != end; ++it)
    {
      UsdPrim prim = *it;

      MObject object = proxy->findRequiredPath(prim.GetPath());

      fileio::translators::TranslatorRefPtr translator = translatorManufacture.get(prim.GetTypeName());
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::updateSchemaPrims: hasEntry(%s, %s)=%b\n", prim.GetPath().GetText(), prim.GetTypeName().GetText(), context->hasEntry(prim.GetPath(), prim.GetTypeName()));

      if(!context->hasEntry(prim.GetPath(), prim.GetTypeName()))
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createSchemaPrims prim=%s hasEntry=false\n", prim.GetPath().GetText());
        AL_BEGIN_PROFILE_SECTION(SchemaPrims);
        if(!fileio::importSchemaPrim(prim, object, 0, context, translator))
        {
          std::cerr << "Error: unable to load schema prim node: '" << prim.GetName().GetString() << "' that has type: '" << prim.GetTypeName() << "'" << std::endl;
        }
        AL_END_PROFILE_SECTION();
      }
      else
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createSchemaPrims [update] prim=%s\n", prim.GetPath().GetText());
        if(translator && translator->update(prim).statusCode() == MStatus::kNotImplemented)
        {
          MGlobal::displayError(
              MString("Prim type has claimed that it supports variant switching via update, but it does not! ") +
              prim.GetPath().GetText());
        }
      }
    }
  }
  AL_END_PROFILE_SECTION();
}


//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::connectSchemaPrims(
    nodes::ProxyShape* proxy,
    const std::vector<UsdPrim>& objsToCreate)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::connectSchemaPrims\n");
  AL_BEGIN_PROFILE_SECTION(PostImportLogic);

  fileio::translators::TranslatorContextPtr context = proxy->context();
  fileio::translators::TranslatorManufacture& translatorManufacture = proxy->translatorManufacture();

  // iterate over the prims we created, and call any post-import logic to make any attribute connections etc
  auto it = objsToCreate.begin();
  const auto end = objsToCreate.end();
  for(; it != end; ++it)
  {
    UsdPrim prim = *it;
    fileio::translators::TranslatorRefPtr torBase = translatorManufacture.get(prim.GetTypeName());
    if(torBase)
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::connectSchemaPrims [postImport] prim=%s\n", prim.GetPath().GetText());
      AL_BEGIN_PROFILE_SECTION(TranslatorBasePostImport);
      torBase->postImport(prim);
      AL_END_PROFILE_SECTION();
    }
  }
  AL_END_PROFILE_SECTION();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePostLoadProcess::initialise(nodes::ProxyShape* ptrNode)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::initialise called\n");

  MFnDagNode fn(ptrNode->thisMObject());
  MDagPath proxyTransformPath;
  fn.getPath(proxyTransformPath);

  // make sure we unload all references prior to reloading them again
  ptrNode->unloadMayaReferences();
  ptrNode->destroyTransformReferences();

  // Now go and delete any child Transforms found directly underneath the shapes parent.
  // These nodes are likely to be driven by the output stage data of the shape.
  {
    MDagModifier modifier;
    MFnDagNode fnDag(fn.parent(0));
    for(uint32_t i = 0; i < fnDag.childCount(); ++i)
    {
      MObject obj = fnDag.child(i);
      if(obj.hasFn(MFn::kPluginTransformNode))
      {
        MFnDagNode fnChild(obj);
        if(fnChild.typeId() == nodes::Transform::kTypeId)
        {
          modifier.deleteNode(obj);
        }
      }
    }

    if(!modifier.doIt())
    {
    }
  }

  AL_BEGIN_PROFILE_SECTION(HuntForNativePrims);
  proxyTransformPath.pop();

  // iterate over the stage and find all custom schema nodes that have registered translator plugins
  std::vector<UsdPrim> schemaPrims;
  std::vector<ImportCallback> callBacks;
  UsdStageRefPtr stage = ptrNode->getUsdStage();
  if(stage)
  {
    huntForNativeNodes(proxyTransformPath, schemaPrims, callBacks, stage, ptrNode->translatorManufacture());
    constructLayers(fn.object(), ptrNode, stage);
  }
  else
  {
    AL_END_PROFILE_SECTION();
    return MS::kSuccess;
  }
  AL_END_PROFILE_SECTION();

  // generate the transform chains
  MObjectToPrim objsToCreate;
  createTranformChainsForSchemaPrims(ptrNode, schemaPrims, proxyTransformPath, objsToCreate);

  // create prims that need to be imported
  createSchemaPrims(ptrNode, schemaPrims);

  // now perform any post-creation fix up
  connectSchemaPrims(ptrNode, schemaPrims);

  // hunt for geometry that can be hidden
  ptrNode->findExcludedGeometry();
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // cmds
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
