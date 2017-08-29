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
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/usd/usdaFileFormat.h"

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
    return status;
  }
}

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(LayerManager, AL_USDMAYA_LAYERMANAGER, AL_usdmaya);

// serialization
MObject LayerManager::m_layers = MObject::kNullObj;
MObject LayerManager::m_identifier = MObject::kNullObj;
MObject LayerManager::m_serialized = MObject::kNullObj;
MObject LayerManager::m_anonymous = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
const LayerManager::layer_identifier::result_type&
LayerManager::layer_identifier::operator()(
    const SdfLayerRefPtr& layer) const
{
    static std::string emptyString;
    return layer ? layer->GetIdentifier() : emptyString;
}

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

    // add attributes to store the serialization info
    m_identifier = addStringAttr("identifier", "id", kCached | kReadable | kStorable | kHidden);
    m_serialized = addStringAttr("serialized", "szd", kCached | kReadable | kStorable | kHidden);
    m_anonymous = addBoolAttr("anonymous", "ann", false, kCached | kReadable | kStorable | kHidden);
    m_layers = addCompoundAttr("layers", "lyr",
        kCached | kReadable | kWritable | kStorable | kConnectable | kHidden | kArray | kUsesArrayDataBuilder,
        {m_identifier, m_serialized, m_anonymous});
  }
  catch(const MStatus& status)
  {
    return status;
  }
  generateAETemplate();
  return MS::kSuccess;
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
MObject LayerManager::findOrCreateNode()
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::findOrCreateNode\n");
  std::lock_guard<std::recursive_mutex> lock(_findNodeMutex);
  MObject theManager = _findNode();
  if (!theManager.isNull())
    return theManager;

  MDGModifier modifier;
  MObject node = modifier.createNode(kTypeId);
  modifier.doIt();
  return node;
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
LayerManager* LayerManager::findOrCreateManager()
{
  return static_cast<LayerManager*>(MFnDependencyNode(findOrCreateNode()).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::addLayer(SdfLayerRefPtr layer)
{
  boost::unique_lock<boost::shared_mutex> lock(m_layersMutex);
  _LayersByLayer& byLayer = m_layerList.get<by_layer>();
  return byLayer.insert(layer).second;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::removeLayer(SdfLayerRefPtr layer)
{
  boost::unique_lock<boost::shared_mutex> lock(m_layersMutex);
  _LayersByLayer& byLayer = m_layerList.get<by_layer>();
  return byLayer.erase(layer) > 0;
}

//----------------------------------------------------------------------------------------------------------------------
SdfLayerHandle LayerManager::findLayer(std::string identifier)
{
  boost::shared_lock_guard<boost::shared_mutex> lock(m_layersMutex);
  _LayersByIdentifier& byIdentifier = m_layerList.get<by_identifier>();
  auto findResult = byIdentifier.find(identifier);
  if(findResult != byIdentifier.end())
  {
    return *findResult;
  }
  return SdfLayerHandle();
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::getLayerIdentifiers(MStringArray& outputNames)
{
  outputNames.clear();
  for(const auto& layer : m_layerList)
  {
    outputNames.append(layer->GetIdentifier().c_str());
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
    MArrayDataBuilder builder(&dataBlock, layers(), m_layerList.size(), &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    std::string temp;
    for (const auto& layer : m_layerList)
    {
      MDataHandle layersElemHandle = builder.addLast(&status);
      AL_MAYA_CHECK_ERROR(status, errorString);
      MDataHandle idHandle = layersElemHandle.child(m_identifier);
      idHandle.setString(convert(layer->GetIdentifier()));
      MDataHandle serializedHandle = layersElemHandle.child(m_serialized);
      layer->ExportToString(&temp);
      serializedHandle.setString(convert(temp));
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
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg(
        "################################################\n"
        "Importing layer:\n"
        "old identifier: %s\n"
        "new identifier: %s\n"
        "format: %s\n"
        "################################################\n"
        "%s\n"
        "################################################\n",
        identifierVal.c_str(),
        layer->GetIdentifier().c_str(),
        layer->GetFileFormat()->GetFormatId().GetText(),
        serializedVal.c_str());
    if(!layer->ImportFromString(serializedVal))
    {
      TF_DEBUG(ALUSDMAYA_LAYERS).Msg("...layer import failed!");
      MGlobal::displayError(MString("Failed to import serialized layer: ") + serializedVal.c_str());
      continue;
    }
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("...layer import succeeded!");
    addLayer(layer);
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
