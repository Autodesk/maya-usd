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
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include "maya/MBoundingBox.h"
#include "maya/MGlobal.h"
#include "maya/MPlugArray.h"
#include "maya/MDGModifier.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MArrayDataBuilder.h"
#include "maya/MArrayDataHandle.h"
#include "maya/MSelectionList.h"

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(Layer, AL_USDMAYA_LAYER, AL_usdmaya);

MObject Layer::m_comment = MObject::kNullObj;
MObject Layer::m_defaultPrim = MObject::kNullObj;
MObject Layer::m_documentation = MObject::kNullObj;
MObject Layer::m_startTime = MObject::kNullObj;
MObject Layer::m_endTime = MObject::kNullObj;
MObject Layer::m_timeCodesPerSecond = MObject::kNullObj;
MObject Layer::m_framePrecision = MObject::kNullObj;
MObject Layer::m_owner = MObject::kNullObj;
MObject Layer::m_sessionOwner = MObject::kNullObj;
MObject Layer::m_permissionToEdit = MObject::kNullObj;
MObject Layer::m_permissionToSave = MObject::kNullObj;

// connection to layers and proxy shapes
MObject Layer::m_subLayers = MObject::kNullObj;
MObject Layer::m_childLayers = MObject::kNullObj;
MObject Layer::m_parentLayer = MObject::kNullObj;
MObject Layer::m_proxyShape = MObject::kNullObj;

// read only identification
MObject Layer::m_displayName = MObject::kNullObj;
MObject Layer::m_realPath = MObject::kNullObj;
MObject Layer::m_fileExtension = MObject::kNullObj;
MObject Layer::m_version = MObject::kNullObj;
MObject Layer::m_repositoryPath = MObject::kNullObj;
MObject Layer::m_assetName = MObject::kNullObj;

// serialisation
MObject Layer::m_nameOnLoad = MObject::kNullObj;
MObject Layer::m_serialized = MObject::kNullObj;
MObject Layer::m_hasBeenEditTarget = MObject::kNullObj;


//----------------------------------------------------------------------------------------------------------------------
MString Layer::toMayaNodeName(std::string name)
{
  for(int i = 0; i < name.size(); ++i)
  {
    const char c = name[i];
    if(c == '.' || c == ' ') name[i] = '_';
  }
  return convert(name);
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::init(ProxyShape* shape, SdfLayerRefPtr handle)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::init %s\n", handle->GetIdentifier().c_str());
  m_shape = shape;
  m_handle = handle;

  // If this layer is the current edit target, flag this as true, so that we know to serialize the layer on file save
  if(shape->usdStage()->GetEditTarget().GetLayer() == handle)
  {
    hasBeenEditTargetPlug().setValue(true);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::postConstructor()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::postConstructor\n");

  MObject nodeRef = thisMObject();
  MPlug displayNamePlug(nodeRef, m_displayName);
  displayNamePlug.setLocked(true);
  MPlug realPathPlug(nodeRef, m_realPath);
  realPathPlug.setLocked(true);
  MPlug fileExtensionPlug(nodeRef, m_fileExtension);
  fileExtensionPlug.setLocked(true);
  MPlug versionPlug(nodeRef, m_version);
  versionPlug.setLocked(true);
  MPlug repositoryPathPlug(nodeRef, m_repositoryPath);
  repositoryPathPlug.setLocked(true);
  MPlug assetNamePlug(nodeRef, m_assetName);
  assetNamePlug.setLocked(true);
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<Layer*> Layer::getChildLayers()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::getChildLayers\n");
  MPlug plug(thisMObject(), m_childLayers);
  std::vector<Layer*> layers;

  // As of Maya 2017, there looks to be a bug in the API where if you save a file containing a message array attribute
  // that has attribute connections, upon reloading the file, MPlug::numElements() will always return 0.
  // Strangely though, querying the connections via MEL will work.
#if 0
  MFnDependencyNode fn;
  for(uint32_t i = 0, num = plug.numElements(); i < num; ++i)
  {
    MPlugArray plugs;
    MStatus status;
    if(plug.elementByLogicalIndex(i).connectedTo(plugs, true, true, &status) && status)
    {
      if(plugs.length())
      {
        if(plugs[0].node().apiType() == MFn::kPluginDependNode)
        {
          if(fn.setObject(plugs[0].node()))
          {
            if(fn.typeId() == Layer::kTypeId)
            {
              layers.push_back((Layer*)fn.userNode());
            }
            else
            {
              MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
            }
          }
          else
          {
            MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
          }
        }
        else
        {
          MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
        }
      }
    }
  }
#else

  MStringArray result;
  MGlobal::executeCommand(MString("listConnections -s 1 -d 1 \"") + plug.name() + "\"", result);

  MSelectionList sl;
  for(int i = 0; i < result.length(); ++i)
  {
    sl.add(result[i]);
  }

  for(int i = 0; i < sl.length(); ++i)
  {
    MObject object;
    sl.getDependNode(i, object);
    MFnDependencyNode fn(object);
    if(fn.typeId() == Layer::kTypeId)
    {
      layers.push_back((Layer*)fn.userNode());
    }
  }

#endif
  return layers;
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<Layer*> Layer::getSubLayers()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::getSubLayers\n");
  MPlug plug(thisMObject(), m_subLayers);
  std::vector<Layer*> layers;

  // As of Maya 2017, there looks to be a bug in the API where if you save a file containing a message array attribute
  // that has attribute connections, upon reloading the file, MPlug::numElements() will always return 0.
  // Strangely though, querying the connections via MEL will work.
#if 0
  MFnDependencyNode fn;
  for(uint32_t i = 0, num = plug.numElements(); i < num; ++i)
  {
    MPlugArray plugs;
    MStatus status;
    if(plug.elementByLogicalIndex(i).connectedTo(plugs, true, true, &status) && status)
    {
      if(plugs.length())
      {
        if(plugs[0].node().apiType() == MFn::kPluginDependNode)
        {
          if(fn.setObject(plugs[0].node()))
          {
            if(fn.typeId() == Layer::kTypeId)
            {
              layers.push_back((Layer*)fn.userNode());
            }
            else
            {
              MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
            }
          }
          else
          {
            MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
          }
        }
        else
        {
          MGlobal::displayError(MString("Invalid connection found on attribute") + plug.elementByLogicalIndex(i).name());
        }
      }
    }
  }
#else
  MStringArray result;
  MGlobal::executeCommand(MString("listConnections -s 1 -d 1 \"") + plug.name() + "\"", result);

  MSelectionList sl;
  for(int i = 0; i < result.length(); ++i)
  {
    sl.add(result[i]);
  }

  for(int i = 0; i < sl.length(); ++i)
  {
    MObject object;
    sl.getDependNode(i, object);
    MFnDependencyNode fn(object);
    if(fn.typeId() == Layer::kTypeId)
    {
      layers.push_back((Layer*)fn.userNode());
    }
  }
#endif
  return layers;
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::buildSubLayers(MDGModifier* pmodifier)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::buildSubLayers\n");
  if(m_shape && m_handle)
  {
    MDGModifier modifier;
    if(!pmodifier) pmodifier = &modifier;

    LAYER_HANDLE_CHECK(m_handle);
    SdfSubLayerProxy subLayers = m_handle->GetSubLayerPaths();
    if(subLayers.empty())
      return;

    for(size_t i = 0, num = subLayers.size(); i < num; ++i)
    {
      // grab the layer identifier
      const std::string identifier = subLayers[i];

      // hunt for the actual layer
      SdfLayerHandle subLayerHandle = SdfLayer::Find(identifier);
      if(subLayerHandle)
      {
        // create a new usdLayer node to reference this layer
        MObject subLayerNode = pmodifier->createNode(Layer::kTypeId);

        // get access to the node pointer
        MFnDependencyNode fn(subLayerNode);
        Layer* subLayerPtr = (Layer*)fn.userNode();

        // now initialise the child layers
        subLayerPtr->init(m_shape, subLayerHandle);

        // go and add the sub layer in to this node
        addSubLayer(subLayerPtr, pmodifier);
      }
    }

    if(pmodifier == &modifier) modifier.doIt();
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool Layer::removeSubLayer(Layer* subLayer)
{
  std::vector<Layer*> layers = getSubLayers();
  MPlug plug(thisMObject(), m_subLayers);
  auto it = std::find(layers.begin(), layers.end(), subLayer);
  if(it != layers.end())
  {
    // hopefully this is the last layer in the array?
    const size_t indexToRemove = it - layers.begin();
    if(indexToRemove == (layers.size() - 1))
    {
      MPlug pplug = subLayer->parentLayerPlug();
      MDGModifier mod;
      mod.disconnect(plug.elementByLogicalIndex(indexToRemove), pplug);
      mod.doIt();

      MDataBlock db = forceCache();
      MStatus status;
      MArrayDataBuilder builder(&db, m_subLayers, 0, &status);
      if(!status)
      {
        std::cout << "failed to attach array builder to attribute" << std::endl;
      }

      MArrayDataHandle handle = db.outputArrayValue(m_subLayers);
      handle.set(builder);
      handle.setClean();
    }

    return true;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------------------------

bool Layer::removeChildLayer(Layer* childLayer)
{
  std::vector<Layer*> layers = getChildLayers();
  MPlug childLayerPlug(thisMObject(), m_childLayers);
  auto it = std::find(layers.begin(), layers.end(), childLayer);

  if(it != layers.end())
  {
    const size_t indexToRemove = it - layers.begin();
    MPlug pplug = childLayer->parentLayerPlug();
    MDGModifier mod;
    mod.disconnect(childLayerPlug.elementByLogicalIndex(indexToRemove), pplug);
    mod.doIt();

    MDataBlock db = forceCache();
    MStatus status;
    MArrayDataBuilder builder(&db, m_childLayers, 0, &status);

    MArrayDataHandle handle = db.outputArrayValue(m_childLayers);
    handle.set(builder);
    handle.setClean();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::addChildLayer(Layer* childLayer, MDGModifier* modifier)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::addChildlLayer\n");
  if(childLayer)
  {
    MPlug plug(thisMObject(), m_childLayers);
    MPlug childLayerParent = childLayer->parentLayerPlug();

    // increase array by one
    uint32_t numElements = plug.numElements();
    plug.setNumElements(numElements + 1);

    // set to child plug
    plug = plug.elementByLogicalIndex(numElements);

    if(modifier)
    {
      modifier->connect(plug, childLayerParent);
    }
    else
    {
      MDGModifier modifier;
      modifier.connect(plug, childLayerParent);
      modifier.doIt();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::addSubLayer(Layer* subLayer, MDGModifier* pmodifier)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::addSubLayer\n");
  if(subLayer)
  {
    MPlug plug(thisMObject(), m_subLayers);
    MPlug subLayerParent = subLayer->parentLayerPlug();

    // increase array by one
    uint32_t numElements = plug.numElements();
    plug.setNumElements(numElements + 1);

    // set to child plug
    plug = plug.elementByLogicalIndex(numElements);

    if(pmodifier)
    {
      pmodifier->connect(plug, subLayerParent);
    }
    else
    {
      MDGModifier modifier;
      modifier.connect(plug, subLayerParent);
      modifier.doIt();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MPlug Layer::parentLayerPlug()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::parentLayerPlug\n");

  if(m_parentLayer != MObject::kNullObj)
  {
    return MPlug(thisMObject(), m_parentLayer);
  }
  return MPlug();
}

//----------------------------------------------------------------------------------------------------------------------
Layer* Layer::getParentLayer()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::getParentLayer\n");
  MPlug plug = parentLayerPlug();
  MPlugArray connections;
  if(plug.connectedTo(connections, true, true))
  {
    MStatus status;
    MFnDependencyNode fn(connections[0].node(), &status);
    if(status)
    {
      return (Layer*)fn.userNode();
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
Layer* Layer::findLayer(SdfLayerHandle handle)
{
  LAYER_HANDLE_CHECK(handle);
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::findLayer: %s\n", handle->GetIdentifier().c_str());
  LAYER_HANDLE_CHECK(m_handle);
  if(m_handle == handle)
  {
    return this;
  }

  Layer* layer = 0;
  if( (layer = findSubLayer(handle)) )
  {
    return layer;
  }
  else if( (layer = findChildLayer(handle)) )
  {
    return layer;
  }
  return 0;
}
//----------------------------------------------------------------------------------------------------------------------
Layer* Layer::findSubLayer(SdfLayerHandle handle)
{
  LAYER_HANDLE_CHECK(handle);
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::findSubLayer: %s\n", handle->GetIdentifier().c_str());
  LAYER_HANDLE_CHECK(m_handle);
  if(m_handle == handle)
  {
    return this;
  }

  std::vector<Layer*> subLayers = getSubLayers();
  for(auto it = subLayers.begin(), end = subLayers.end(); it != end; ++it)
  {
    if(*it)
    {
      Layer* layer = (*it)->findLayer(handle);
      if(layer)
      {
        return layer;
      }
    }
  }

  return 0;
}
//----------------------------------------------------------------------------------------------------------------------
Layer* Layer::findChildLayer(SdfLayerHandle handle)
{
  LAYER_HANDLE_CHECK(handle);
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::findChildLayer: %s\n", handle->GetIdentifier().c_str());
  LAYER_HANDLE_CHECK(m_handle);
  if(m_handle == handle)
  {
    return this;
  }

  std::vector<Layer*> childLayers = getChildLayers();
  for(auto it = childLayers.begin(), end = childLayers.end(); it != end; ++it)
  {
    if(*it)
    {
      Layer* layer = (*it)->findLayer(handle);
      if(layer)
      {
        return layer;
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Layer::initialise()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::initialise\n");
  try
  {
    setNodeType(kTypeName);
    addFrame("USD Layer Info");

    // do not write these nodes to the file. They will be created automagically by the proxy shape
    m_comment = addStringAttr("comment", "cm", kReadable | kWritable | kInternal);
    m_defaultPrim = addStringAttr("defaultPrim", "dp", kReadable | kWritable | kInternal);
    m_documentation = addStringAttr("documentation", "docs", kReadable | kWritable | kInternal);
    m_startTime = addDoubleAttr("startTime", "stc", 0, kReadable | kWritable | kInternal);
    m_endTime = addDoubleAttr("endTime", "etc", 0, kReadable | kWritable | kInternal);
    m_timeCodesPerSecond = addDoubleAttr("timeCodesPerSecond", "tcps", 0, kReadable | kWritable | kInternal);
    m_framePrecision = addInt32Attr("framePrecision", "fp", 0, kReadable | kWritable | kInternal);
    m_owner = addStringAttr("owner", "own", kReadable | kWritable | kInternal);
    m_sessionOwner = addStringAttr("sessionOwner", "sho", kReadable | kWritable | kInternal);
    m_permissionToEdit = addBoolAttr("permissionToEdit", "pte", false, kReadable | kWritable | kInternal);
    m_permissionToSave = addBoolAttr("permissionToSave", "pts", false, kReadable | kWritable | kInternal);

    // parent/child relationships
    m_proxyShape = addMessageAttr("proxyShape", "psh", kConnectable | kReadable | kWritable | kHidden | kStorable);
    m_subLayers = addMessageAttr("subLayers", "sl", kConnectable | kReadable | kWritable | kHidden | kArray | kUsesArrayDataBuilder | kStorable);
    m_parentLayer = addMessageAttr("parentLayer", "pl", kConnectable | kReadable | kWritable | kHidden | kStorable);
    m_childLayers = addMessageAttr("childLayer", "cl", kConnectable | kReadable | kWritable | kHidden | kArray | kUsesArrayDataBuilder | kStorable);

    addFrame("USD Layer Identification");
    m_displayName = addStringAttr("displayName", "dn", kReadable | kWritable | kInternal);
    m_realPath = addStringAttr("realPath", "rp", kReadable | kWritable | kInternal);
    m_fileExtension = addStringAttr("fileExtension", "fe", kReadable | kWritable | kInternal);
    m_version = addStringAttr("version", "ver", kWritable | kReadable | kInternal);
    m_repositoryPath = addStringAttr("repositoryPath", "rpath", kReadable | kWritable | kInternal);
    m_assetName = addStringAttr("assetName", "an", kReadable | kWritable | kInternal);

    // add attributes to store the serialisation info
    m_serialized = addStringAttr("serialised", "szd", kReadable | kWritable | kStorable | kHidden);
    m_nameOnLoad = addStringAttr("nameOnLoad", "nol", kReadable | kWritable | kStorable | kHidden);
    m_hasBeenEditTarget = addBoolAttr("hasBeenEditTarget", "hbet", false, kReadable | kWritable | kStorable | kHidden);
  }
  catch(const MStatus& status)
  {
    return status;
  }
  generateAETemplate();
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool Layer::getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::getInternalValueInContext %s\n", plug.name().asChar());
  if(!m_handle)
    return false;

  // something has gone terribly wrong if these are null, yet the handle remains valid.
  assert(m_shape && m_shape->usdStage());

  if(plug == m_displayName)
  {
    dataHandle.setString(convert(m_handle->GetDisplayName()));
  }
  else
  if(plug == m_realPath)
  {
    dataHandle.setString(convert(m_handle->GetRealPath()));
  }
  else
  if(plug == m_fileExtension)
  {
    dataHandle.setString(convert(m_handle->GetFileExtension()));
  }
  else
  if(plug == m_version)
  {
    dataHandle.setString(convert(m_handle->GetVersion()));
  }
  else
  if(plug == m_repositoryPath)
  {
    dataHandle.setString(convert(m_handle->GetRepositoryPath()));
  }
  else
  if(plug == m_assetName)
  {
    dataHandle.setString(convert(m_handle->GetAssetName()));
  }
  else
  if(plug == m_comment)
  {
    dataHandle.setString(convert(m_handle->GetComment()));
  }
  else
  if(plug == m_defaultPrim)
  {
    if(m_handle->HasDefaultPrim())
      dataHandle.setString(convert(m_handle->GetDefaultPrim().GetString()));
    else
      dataHandle.setString("");
  }
  else
  if(plug == m_documentation)
  {
    dataHandle.set(convert(m_handle->GetDocumentation()));
  }
  else
  if(plug == m_startTime)
  {
    dataHandle.set(m_handle->HasStartTimeCode() ? m_handle->GetStartTimeCode() : 0.0);
  }
  else
  if(plug == m_endTime)
  {
    dataHandle.set(m_handle->HasEndTimeCode() ? m_handle->GetEndTimeCode() : 0.0);
  }
  else
  if(plug == m_timeCodesPerSecond)
  {
    dataHandle.set(m_handle->HasTimeCodesPerSecond() ? m_handle->GetTimeCodesPerSecond() : 0.0);
  }
  else
  if(plug == m_framePrecision)
  {
    dataHandle.set(m_handle->GetFramePrecision());
  }
  else
  if(plug == m_owner)
  {
    if(m_handle->HasOwner())
    {
      dataHandle.set(convert(m_handle->GetOwner()));
    }
    else
    {
      MString empty;
      dataHandle.set(empty);
    }
  }
  else
  if(plug == m_sessionOwner)
  {
    if(m_handle->HasSessionOwner())
    {
      dataHandle.set(convert(m_handle->GetSessionOwner()));
    }
    else
    {
      MString empty;
      dataHandle.set(empty);
    }
  }
  else
  if(plug == m_permissionToEdit)
  {
    dataHandle.set(m_handle->PermissionToEdit());
  }
  else
  if(plug == m_permissionToSave)
  {
    dataHandle.set(m_handle->PermissionToSave());
  }
  else
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool Layer::setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::setInternalValueInContext %s\n", plug.name().asChar());
  if(!m_handle)
    return false;

  // something has gone terribly wrong if these are null, yet the handle remains valid.
  assert(m_shape && m_shape->usdStage());

  if(plug == m_comment)
  {
    m_handle->SetComment(convert(dataHandle.asString()));
  }
  else
  if(plug == m_defaultPrim)
  {
    SdfPath primPath(convert(dataHandle.asString()));
    UsdPrim prim = m_shape->usdStage()->GetPrimAtPath(primPath);
    if(prim)
    {
      m_handle->SetDefaultPrim(prim.GetName());
    }
    else
    {
      return false;
    }
  }
  else
  if(plug == m_documentation)
  {
    m_handle->SetDocumentation(convert(dataHandle.asString()));
  }
  else
  if(plug == m_startTime)
  {
    m_handle->SetStartTimeCode(dataHandle.asDouble());
  }
  else
  if(plug == m_endTime)
  {
    m_handle->SetEndTimeCode(dataHandle.asDouble());
  }
  else
  if(plug == m_timeCodesPerSecond)
  {
    m_handle->SetTimeCodesPerSecond(dataHandle.asDouble());
  }
  else
  if(plug == m_framePrecision)
  {
    m_handle->SetFramePrecision(dataHandle.asInt());
  }
  else
  if(plug == m_owner)
  {
    m_handle->SetOwner(convert(dataHandle.asString()));
  }
  else
  if(plug == m_sessionOwner)
  {
    m_handle->SetSessionOwner(convert(dataHandle.asString()));
  }
  else
  if(plug == m_permissionToEdit)
  {
    m_handle->SetPermissionToEdit(dataHandle.asBool());
  }
  else
  if(plug == m_permissionToSave)
  {
    m_handle->SetPermissionToSave(dataHandle.asBool());
  }
  else
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool Layer::hasBeenTheEditTarget() const
{
  MPlug plug(thisMObject(), m_hasBeenEditTarget);
  return plug.asBool();
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::setHasBeenTheEditTarget(bool value)
{
  MPlug plug(thisMObject(), m_hasBeenEditTarget);
  plug.setValue(value);
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::setLayerAndClearAttribute(SdfLayerHandle handle)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::setLayerAndClearAttribute\n");
  m_handle = handle;
  if(m_handle)
  {
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg(" - handle valid\n");
    const MString serializedLayer = serializedPlug().asString();
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("data\n%s\n", serializedLayer.asChar());
    if(serializedLayer.length())
    {
      TF_DEBUG(ALUSDMAYA_LAYERS).Msg("importing\n");
      if(!m_handle->ImportFromString(convert(serializedLayer)))
      {
        MGlobal::displayError(MString("Failed to import serialized layer: ") + serializedLayer);
      }
      serializedPlug().setValue(MString());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Layer::populateSerialisationAttributes()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::populateSerialisationAttributes: %s %b", m_handle->GetDisplayName().c_str(), hasBeenTheEditTarget());
  if(hasBeenTheEditTarget() && m_handle)
  {
    nameOnLoadPlug().setValue(realPathPlug().asString());

    std::string temp;
    m_handle->ExportToString(&temp);
    serializedPlug().setValue(convert(temp));
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::populateSerialisationAttributes -> contents\n%s\n", temp.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
