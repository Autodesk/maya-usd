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
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/maya/utils/Utils.h"
#include "AL/maya/utils/MayaHelperMacros.h"

#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usdImaging/usdImaging/version.h"
#if (USD_IMAGING_API_VERSION >= 7)
  #include "pxr/usdImaging/usdImagingGL/gl.h"
  #include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#else
  #include "pxr/usdImaging/usdImaging/gl.h"
  #include "pxr/usdImaging/usdImaging/hdEngine.h"
#endif

#include "maya/MBoundingBox.h"
#include "maya/MGlobal.h"
#include "maya/MPlugArray.h"
#include "maya/MDGModifier.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MArrayDataBuilder.h"
#include "maya/MArrayDataHandle.h"
#include "maya/MSelectionList.h"
#include "maya/MItDependencyNodes.h"

#include <boost/thread.hpp>
#include <boost/thread/shared_lock_guard.hpp>

#include <mutex>

namespace {
  // Global mutex protecting _findNode / findOrCreateNode.
  // Recursive because we need to get the mutex inside of conditionalCreator,
  // but that may be triggered by the node creation inside of findOrCreateNode.

  // Note on layerManager / multithreading:
  // I don't know that layerManager will be used in a multihreaded manenr... but I also don't know it COULDN'T be.
  // (I haven't really looked into the way maya's new multi-threaded node evaluation works, for instance.) This is
  // essentially a globally shared resource, so I figured better be safe...
  static std::recursive_mutex _findNodeMutex;


  // Utility func to disconnect an array plug, and all it's element plugs, and all
  // their child plugs.
  // Not in Utils, because it's not generic - ie, doesn't handle general case
  // where compound/array plugs may be nested arbitrarily deep...
  MStatus disconnectCompoundArrayPlug(MPlug arrayPlug)
  {
    const char* errorString = "disconnectCompoundArrayPlug";
    MStatus status;
    MPlug elemPlug;
    MPlug srcPlug;
    MPlugArray destPlugs;
    MDGModifier dgmod;

    auto disconnectPlug = [&](MPlug plug) -> MStatus {
      MStatus status;
      srcPlug = plug.source(&status);
      AL_MAYA_CHECK_ERROR(status, errorString);
      if(!srcPlug.isNull())
      {
        dgmod.disconnect(srcPlug, plug);
      }
      destPlugs.clear();
      plug.destinations(destPlugs, &status);
      AL_MAYA_CHECK_ERROR(status, errorString);
      for(size_t i=0; i < destPlugs.length(); ++i)
      {
        dgmod.disconnect(plug, destPlugs[i]);
      }
      return status;
    };

    // Considered using numConnectedElements, but for arrays-of-compound attributes, not sure if this will
    // also detect connections to a child-of-an-element... so just iterating through all plugs. Shouldn't
    // be too many...
    const size_t numElements = arrayPlug.evaluateNumElements();
    // Iterate over all elements...
    for(size_t elemI = 0; elemI < numElements; ++elemI)
    {
      elemPlug = arrayPlug.elementByPhysicalIndex(elemI, &status);

      // Disconnect the element compound attribute
      AL_MAYA_CHECK_ERROR(status, errorString);
      AL_MAYA_CHECK_ERROR(disconnectPlug(elemPlug), errorString);

      // ...then disconnect any children
      if(elemPlug.numConnectedChildren() > 0)
      {
        for(size_t childI = 0; childI < elemPlug.numChildren(); ++childI)
        {
          AL_MAYA_CHECK_ERROR(disconnectPlug(elemPlug.child(childI)), errorString);
        }
      }
    }
    return dgmod.doIt();
  }
}

namespace AL {
namespace usdmaya {
namespace nodes {

LayerManager::~LayerManager()
{
  removeAttributeChangedCallback();
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerDatabase::addLayer(SdfLayerRefPtr layer, const std::string& identifier)
{
  auto insertLayerResult = m_layerToIds.emplace(std::piecewise_construct,
      std::forward_as_tuple(layer),
      std::forward_as_tuple());
  auto& idsForLayer = insertLayerResult.first->second;

  _addLayer(layer, layer->GetIdentifier(), idsForLayer);
  if (identifier != layer->GetIdentifier() && !identifier.empty())
  {
    _addLayer(layer, identifier, idsForLayer);
  }
  return insertLayerResult.second;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerDatabase::removeLayer(SdfLayerRefPtr layer)
{
  auto foundLayerAndIds = m_layerToIds.find(layer);
  if (foundLayerAndIds == m_layerToIds.end()) return false;

  for (std::string& oldId : foundLayerAndIds->second)
  {
    auto oldIdPosition = m_idToLayer.find(oldId);
#ifdef DEBUG
    assert (oldIdPosition != m_idToLayer.end());
#else
    if (oldIdPosition == m_idToLayer.end())
    {
      MGlobal::displayError(MString("Error - layer '") + AL::maya::utils::convert(layer->GetIdentifier())
          + "' could be found indexed by layer, but not by identifier '"
          + AL::maya::utils::convert(oldId) + "'");
    }
    else
#endif // DEBUG
    {
      m_idToLayer.erase(oldIdPosition);
    }
  }
  m_layerToIds.erase(foundLayerAndIds);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
SdfLayerHandle LayerDatabase::findLayer(std::string identifier) const
{
  auto foundIdAndLayer = m_idToLayer.find(identifier);
  if(foundIdAndLayer != m_idToLayer.end())
  {
    return foundIdAndLayer->second;
  }


  return SdfLayerHandle();
}

//----------------------------------------------------------------------------------------------------------------------
void LayerDatabase::_addLayer(SdfLayerRefPtr layer, const std::string& identifier,
    std::vector<std::string>& idsForLayer)
{
  // Try to insert into m_idToLayer...
  auto insertIdResult = m_idToLayer.emplace(identifier, layer);
  if (!insertIdResult.second)
  {
    // We've seen this identifier before...
    if (insertIdResult.first->second == layer)
    {
      // ...and it was referring to the same layer. Nothing to do!
      return;
    }

    // If it was pointing to a DIFFERENT layer, we need to first remove
    // this id from the set of ids for the OLD layer...
    SdfLayerRefPtr oldLayer = insertIdResult.first->second;
    auto oldLayerAndIds = m_layerToIds.find(oldLayer);
#ifdef DEBUG
    assert (oldLayerAndIds != m_layerToIds.end());
#else
    if (oldLayerAndIds == m_layerToIds.end())
    {
      // The layer didn't exist in the opposite direction - this should
      // never happen, but don't want to crash if it does
      MGlobal::displayError(MString("Error - layer '") + AL::maya::utils::convert(identifier)
          + "' could be found indexed by identifier, but not by layer");
    }
    else
#endif // DEBUG
    {
      auto& oldLayerIds = oldLayerAndIds->second;
      if (oldLayerIds.size() <= 1)
      {
        // This was the ONLY identifier for the layer - so delete
        // the layer entirely!
        m_layerToIds.erase(oldLayerAndIds);
      }
      else
      {
        auto idLocation = std::find(oldLayerIds.begin(), oldLayerIds.end(),
            identifier);
#ifdef DEBUG
        assert (idLocation != oldLayerIds.end());
#else
        if(idLocation == oldLayerIds.end())
        {
          MGlobal::displayError(MString("Error - layer '") + AL::maya::utils::convert(identifier)
              + "' could be found indexed by identifier, but was not in layer's list of identifiers");
        }
        else
#endif
        {
          oldLayerIds.erase(idLocation);
        }
      }
    }

    // Ok, we've cleaned up the OLD layer - now make the id point to our
    // NEW layer
    insertIdResult.first->second = layer;
  }

  // Ok, we've now added the layer to m_idToLayer, and cleaned up
  // any potential old entries from m_layerToIds. Now we just need
  // to add the identifier to idsForLayer (which should be == m_layerToIds[layer])
  idsForLayer.push_back(identifier);
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(LayerManager, AL_USDMAYA_LAYERMANAGER, AL_usdmaya);

// serialization
MObject LayerManager::m_layers = MObject::kNullObj;
MObject LayerManager::m_identifier = MObject::kNullObj;
MObject LayerManager::m_serialized = MObject::kNullObj;
MObject LayerManager::m_anonymous = MObject::kNullObj;
MObject LayerManager::m_rendererPlugin = MObject::kNullObj;

TfTokenVector LayerManager::m_rendererPluginsTokens;
MStringArray LayerManager::m_rendererPluginsNames;

//----------------------------------------------------------------------------------------------------------------------
void* LayerManager::conditionalCreator()
{
  // If we were called from findOrCreate, we don't need to call findNode, we already did
  MObject theManager = findNode();
  if (!theManager.isNull())
  {
    MFnDependencyNode fn(theManager);
    MGlobal::displayError(MString("cannot create a new '") + kTypeName + "' node, an unreferenced"
        " one already exists: " + fn.name());
    return nullptr;
  }
  return creator();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerManager::initialise()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::initialize\n");
  try
  {
    setNodeType(kTypeName);
    addFrame("USD Layer Manager Node");

    addFrame("Serialization infos");

    // add attributes to store the serialization info
    m_identifier = addStringAttr("identifier", "id", kCached | kReadable | kStorable | kHidden);
    m_serialized = addStringAttr("serialized", "szd", kCached | kReadable | kStorable | kHidden);
    m_anonymous = addBoolAttr("anonymous", "ann", false, kCached | kReadable | kStorable | kHidden);
    m_layers = addCompoundAttr("layers", "lyr",
        kCached | kReadable | kWritable | kStorable | kConnectable | kHidden | kArray | kUsesArrayDataBuilder,
        {m_identifier, m_serialized, m_anonymous});

    // Create dummy imaging engine to get renderer names
    UsdImagingGL imagingEngine(SdfPath(), {});
    m_rendererPluginsTokens = imagingEngine.GetRendererPlugins();
    const size_t numTokens = m_rendererPluginsTokens.size();
    m_rendererPluginsNames = MStringArray(numTokens, MString());

    // as it's not clear what is the liftime of strings returned from HdEngine::GetRendererPluginDesc
    // we store them in array to populate enum attribute also we store array of MString names for menu items
    std::vector<std::string> pluginNames(numTokens);
    std::vector<const char*> enumNames(numTokens + 1, nullptr);
    std::vector<int16_t> enumIds(numTokens + 1, -1);
    for (size_t i = 0; i < numTokens; ++i)
    {
      pluginNames[i] = imagingEngine.GetRendererPluginDesc(m_rendererPluginsTokens[i]);
      m_rendererPluginsNames[i] = MString(pluginNames[i].data(), pluginNames[i].size());
      enumNames[i] = pluginNames[i].data();
      enumIds[i] = i;
    }

    m_rendererPlugin = addEnumAttr(
      "rendererPlugin", "rp", kCached | kReadable | kWritable , enumNames.data(), enumIds.data()
    );

  }
  catch(const MStatus& status)
  {
    return status;
  }
  generateAETemplate();
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::onAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug&, void* clientData)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::onAttributeChanged\n");
  LayerManager* manager = static_cast<LayerManager*>(clientData);
  assert(manager);
  if(plug == m_rendererPlugin)
  {
    // Find all proxy shapes and change renderer plugin
    MFnDependencyNode fn;
    MItDependencyNodes iter(MFn::kPluginShape);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == ProxyShape::kTypeId)
      {
        ProxyShape* proxy = static_cast<ProxyShape*>(fn.userNode());
        manager->changeRendererPlugin(proxy);
      }
    }
    //! We need to refresh viewport to changes take effect
    MGlobal::executeCommandOnIdle("refresh -force");
  }
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::removeAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::removeAttributeChangedCallback\n");
  if(m_attributeChanged != -1)
  {
    MMessage::removeCallback(m_attributeChanged);
    m_attributeChanged = -1;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::addAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::addAttributeChangedCallback\n");
  if(m_attributeChanged == -1)
  {
    MObject obj = thisMObject();
    m_attributeChanged = MNodeMessage::addAttributeChangedCallback(obj, onAttributeChanged, (void*)this);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::postConstructor()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::postConstructor\n");
  addAttributeChangedCallback();
}

//----------------------------------------------------------------------------------------------------------------------
MObject LayerManager::findNode()
{
  std::lock_guard<std::recursive_mutex> lock(_findNodeMutex);
  return _findNode();
}

//----------------------------------------------------------------------------------------------------------------------
MObject LayerManager::_findNode()
{
  MFnDependencyNode fn;
  MItDependencyNodes iter(MFn::kPluginDependNode);
  for(; !iter.isDone(); iter.next())
  {
    MObject mobj = iter.item();
    fn.setObject(mobj);
    if(fn.typeId() == kTypeId && !fn.isFromReferencedFile())
    {
      return mobj;
    }
  }
  return MObject::kNullObj;
}


//----------------------------------------------------------------------------------------------------------------------
// TODO: make it take an optional MDGModifier
MObject LayerManager::findOrCreateNode(MDGModifier* dgmod, bool* wasCreated)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::findOrCreateNode\n");
  std::lock_guard<std::recursive_mutex> lock(_findNodeMutex);
  MObject theManager = _findNode();

  if (!theManager.isNull())
  {
    if (wasCreated) *wasCreated = false;
    return theManager;
  }

  if (wasCreated) *wasCreated = true;

  if (dgmod)
  {
    return dgmod->createNode(kTypeId);
  }
  else
  {
    MDGModifier modifier;
    MObject node = modifier.createNode(kTypeId);
    modifier.doIt();
    return node;
  }
}

//----------------------------------------------------------------------------------------------------------------------
LayerManager* LayerManager::findManager()
{
  MObject manager = findNode();
  if(manager.isNull())
  {
    return nullptr;
  }
  return static_cast<LayerManager*>(MFnDependencyNode(manager).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
LayerManager* LayerManager::findOrCreateManager(MDGModifier* dgmod, bool* wasCreated)
{
  return static_cast<LayerManager*>(MFnDependencyNode(findOrCreateNode(dgmod,
      wasCreated)).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::addLayer(SdfLayerHandle layer, const std::string& identifier)
{
  SdfLayerRefPtr layerRef(layer);
  if (!layerRef)
  {
    MGlobal::displayError("LayerManager::addLayer - given layer is no longer valid");
    return false;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_layersMutex);
  return m_layerDatabase.addLayer(layerRef, identifier);
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::removeLayer(SdfLayerHandle layer)
{
  SdfLayerRefPtr layerRef(layer);
  if (!layerRef)
  {
    MGlobal::displayError("LayerManager::removeLayer - given layer is no longer valid");
    return false;
  }
  boost::unique_lock<boost::shared_mutex> lock(m_layersMutex);
  return m_layerDatabase.removeLayer(layerRef);
}

//----------------------------------------------------------------------------------------------------------------------
SdfLayerHandle LayerManager::findLayer(std::string identifier)
{
  boost::shared_lock_guard<boost::shared_mutex> lock(m_layersMutex);
  return m_layerDatabase.findLayer(identifier);
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::getLayerIdentifiers(MStringArray& outputNames)
{
  outputNames.clear();
  for(const auto& layerAndIds : m_layerDatabase)
  {
    outputNames.append(layerAndIds.first->GetIdentifier().c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerManager::populateSerialisationAttributes()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::populateSerialisationAttributes\n");
  const char* errorString = "LayerManager::populateSerialisationAttributes";

  MStatus status;
  MPlug arrayPlug = layersPlug();

  // First, disconnect any connected attributes
  AL_MAYA_CHECK_ERROR(disconnectCompoundArrayPlug(arrayPlug), errorString);

  // Then fill out the array attribute
  MDataBlock dataBlock = forceCache();

  MArrayDataHandle layersArrayHandle = dataBlock.outputArrayValue(m_layers, &status);
  AL_MAYA_CHECK_ERROR(status, errorString);
  {
    boost::shared_lock_guard<boost::shared_mutex> lock(m_layersMutex);
    MArrayDataBuilder builder(&dataBlock, layers(), m_layerDatabase.size(), &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    std::string temp;
    for (const auto& layerAndIds : m_layerDatabase)
    {
      auto& layer = layerAndIds.first;
      MDataHandle layersElemHandle = builder.addLast(&status);
      AL_MAYA_CHECK_ERROR(status, errorString);
      MDataHandle idHandle = layersElemHandle.child(m_identifier);
      idHandle.setString(AL::maya::utils::convert(layer->GetIdentifier()));
      MDataHandle serializedHandle = layersElemHandle.child(m_serialized);
      layer->ExportToString(&temp);
      serializedHandle.setString(AL::maya::utils::convert(temp));
      MDataHandle anonHandle = layersElemHandle.child(m_anonymous);
      anonHandle.setBool(layer->IsAnonymous());
    }
    AL_MAYA_CHECK_ERROR(layersArrayHandle.set(builder), errorString);
  }
  AL_MAYA_CHECK_ERROR(layersArrayHandle.setAllClean(), errorString);
  AL_MAYA_CHECK_ERROR(dataBlock.setClean(layers()), errorString);
  return status;
}

MStatus LayerManager::clearSerialisationAttributes()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::clearSerialisationAttributes\n");
  const char* errorString = "LayerManager::clearSerialisationAttributes";

  MStatus status;
  MPlug arrayPlug = layersPlug();

  // First, disconnect any connected attributes
  AL_MAYA_CHECK_ERROR(disconnectCompoundArrayPlug(arrayPlug), errorString);

  // Then wipe the array attribute
  MDataBlock dataBlock = forceCache();
  MArrayDataHandle layersArrayHandle = dataBlock.outputArrayValue(m_layers, &status);
  AL_MAYA_CHECK_ERROR(status, errorString);

  MArrayDataBuilder builder(&dataBlock, layers(), 0, &status);
  AL_MAYA_CHECK_ERROR(status, errorString);
  AL_MAYA_CHECK_ERROR(layersArrayHandle.set(builder), errorString);
  AL_MAYA_CHECK_ERROR(layersArrayHandle.setAllClean(), errorString);
  AL_MAYA_CHECK_ERROR(dataBlock.setClean(layers()), errorString);
  return status;
}

void LayerManager::loadAllLayers()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::loadAllLayers\n");
  const char* errorString = "LayerManager::loadAllLayers";

  MStatus status;
  MPlug allLayersPlug = layersPlug();
  MPlug singleLayerPlug;
  MPlug idPlug;
  MPlug anonymousPlug;
  MPlug serializedPlug;
  std::string identifierVal;
  std::string serializedVal;
  SdfLayerRefPtr layer;
  // We DON'T want to use evaluate num elements, because we don't want to trigger
  // a compute - we want the value(s) as read from the file!
  const unsigned int numElements = allLayersPlug.numElements();
  for(unsigned int i=0; i < numElements; ++i)
  {
    singleLayerPlug = allLayersPlug.elementByPhysicalIndex(i, &status);
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    idPlug = singleLayerPlug.child(m_identifier, &status);
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    anonymousPlug = singleLayerPlug.child(m_anonymous, &status);
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    serializedPlug = singleLayerPlug.child(m_serialized, &status);
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);

    identifierVal = idPlug.asString(MDGContext::fsNormal, &status).asChar();
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    if(identifierVal.empty())
    {
      MGlobal::displayError(MString("Error - plug ") + idPlug.partialName(true) + "had empty identifier");
      continue;
    }
    serializedVal = serializedPlug.asString(MDGContext::fsNormal, &status).asChar();
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    if(serializedVal.empty())
    {
      MGlobal::displayError(MString("Error - plug ") + serializedPlug.partialName(true) + "had empty serialization");
      continue;
    }

    bool isAnon = anonymousPlug.asBool(MDGContext::fsNormal, &status);
    AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
    if(isAnon)
    {
      // Note that the new identifier will not match the old identifier - only the "tag" will be retained
      layer = SdfLayer::CreateAnonymous(SdfLayer::GetDisplayNameFromIdentifier(identifierVal));
    }
    else
    {
      SdfLayerHandle layerHandle = SdfLayer::Find(identifierVal);
      if (layerHandle)
      {
        layer = layerHandle;
      }
      else
      {
        // TODO: currently, there is a small window here, after the find, and before the New, where
        // another process might sneak in and create a layer with the same identifier, which could cause
        // an error. This seems unlikely, but we have a discussion with Pixar to find a way to avoid this.

        SdfFileFormatConstPtr fileFormat;
        if(TfStringStartsWith(serializedVal, "#usda "))
        {
          fileFormat = SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id);
        }
        else
        {
          fileFormat = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
        }
        layer = SdfLayer::New(fileFormat, identifierVal);
        if (!layer)
        {
          MGlobal::displayError(MString("Error - failed to create new layer for identifier '") + identifierVal.c_str()
                        + "' for plug " + idPlug.partialName(true));
          continue;
        }
      }
    }

    // Don't print the entirety of layers > ~1MB
    constexpr int MAX_LAYER_CHARS=1000000;

    TF_DEBUG(ALUSDMAYA_LAYERS).Msg(
        "################################################\n"
        "Importing layer:\n"
        "old identifier: %s\n"
        "new identifier: %s\n"
        "format: %s\n"
        "################################################\n"
        "%.*s\n%s"
        "################################################\n",
        identifierVal.c_str(),
        layer->GetIdentifier().c_str(),
        layer->GetFileFormat()->GetFormatId().GetText(),
        MAX_LAYER_CHARS,
        serializedVal.c_str(),
        serializedVal.length() > MAX_LAYER_CHARS ? "<truncated>\n" : ""
        );
    if(!layer->ImportFromString(serializedVal))
    {
      TF_DEBUG(ALUSDMAYA_LAYERS).Msg("...layer import failed!\n");
      MGlobal::displayError(MString("Failed to import serialized layer: ") + serializedVal.c_str());
      continue;
    }
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("...layer import succeeded!\n");
    addLayer(layer, identifierVal);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::changeRendererPlugin(ProxyShape* proxy, bool creation)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::changeRendererPlugin\n");
  assert(proxy);
  if (proxy->engine())
  {
    MPlug plug(thisMObject(), m_rendererPlugin);
    short rendererId = plug.asShort();
    if (rendererId < m_rendererPluginsTokens.size())
    {
      // Skip redundant renderer changes on ProxyShape creation
      if (rendererId == 0 && creation)
        return;
      
      TfToken plugin = m_rendererPluginsTokens[rendererId];
      if (!proxy->engine()->SetRendererPlugin(plugin))
      {
        MString data(plugin.data());
        MGlobal::displayError(MString("Failed to set renderer plugin: ") + data);
      }
    }
    else
    {
      MString data;
      data.set(rendererId);
      MGlobal::displayError(MString("Invalid renderer plugin index: ") + data);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
size_t LayerManager::getRendererPluginIndex() const
{
  MPlug plug(thisMObject(), m_rendererPlugin);
  return plug.asShort();
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::setRendererPlugin(const MString& pluginName)
{
  int index = m_rendererPluginsNames.indexOf(pluginName);
  if (index >= 0)
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::setRendererPlugin\n");
    MPlug plug(thisMObject(), m_rendererPlugin);
    plug.setShort(index);
    return true;
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Failed to set renderer plugin!\n");
    MGlobal::displayError(MString("Failed to set renderer plugin: ") + pluginName);
  }
  return false;
}


//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
