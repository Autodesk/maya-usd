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
#include "AL/usdmaya/Global.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include "maya/MGlobal.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MItDependencyNodes.h"

#include <iostream>

#ifndef AL_USDMAYA_LOCATION_NAME
  #define AL_USDMAYA_LOCATION_NAME "AL_USDMAYA_LOCATION"
#endif

namespace AL {
namespace usdmaya {


//----------------------------------------------------------------------------------------------------------------------
MCallbackId Global::m_preSave;
MCallbackId Global::m_postSave;
MCallbackId Global::m_preOpen;
MCallbackId Global::m_postOpen;
MCallbackId Global::m_fileNew;

//----------------------------------------------------------------------------------------------------------------------
static void onFileNew(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("onFileNew\n");
  // These should both clear the caches, however they don't actually do anything of the sort. Puzzled.
  UsdUtilsStageCache::Get().Clear();
  StageCache::Clear();
}

//----------------------------------------------------------------------------------------------------------------------
static void preFileOpen(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("preFileOpen\n");
}

//----------------------------------------------------------------------------------------------------------------------
static void postFileOpen(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("postFileOpen\n");

  MFnDependencyNode fn;
  {
    MItDependencyNodes iter(MFn::kPluginShape);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == nodes::ProxyShape::kTypeId)
      {
        // execute a pull on each proxy shape to ensure that each one has a valid USD stage!
        nodes::ProxyShape* proxy = (nodes::ProxyShape*)fn.userNode();
        auto stage = proxy->getUsdStage();
        proxy->deserialiseTranslatorContext();
        proxy->findTaggedPrims();
        proxy->constructGLImagingEngine();
        proxy->deserialiseTransformRefs();
        auto layer = proxy->getLayer();
        if(layer)
        {
          layer->setLayerAndClearAttribute(stage->GetSessionLayer());
        }
      }
    }
  }
  {
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == nodes::Layer::kTypeId)
      {
        // now go and fix up each of the layer nodes in the scene
        nodes::Layer* layerPtr = (nodes::Layer*)fn.userNode();
        MPlug plug = layerPtr->nameOnLoadPlug();
        MString path = plug.asString();
        if(path.length() && path.substring(0, 3) != "anon")
        {
          SdfLayerHandle layer = SdfLayer::FindOrOpen(path.asChar());
          LAYER_HANDLE_CHECK(layer);
          layerPtr->setLayerAndClearAttribute(layer);
        }
        else
        {
        }
      }
    }
  }
  {
    MItDependencyNodes iter(MFn::kPluginTransformNode);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == nodes::Transform::kTypeId)
      {
        // ensure all of the transforms are referring to the correct prim
        nodes::Transform* tmPtr = (nodes::Transform*)fn.userNode();
        tmPtr->transform()->initialiseToPrim(true, tmPtr);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
static void preFileSave(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("preFileSave\n");

  // currently, if we have selected a shape in the usd proxy shape, a series of transforms will have been created.
  // Ideally we don't want these transient nodes to be stored in the Maya file, so make sure we unselect prior to a file
  // save (which should call another set of callbacks and delete those transient nodes. This should leave us with just
  // those AL::usdmaya::nodes::Transform nodes that are created because they are required, or have been requested).
  MGlobal::clearSelectionList();
}

//----------------------------------------------------------------------------------------------------------------------
static void postFileSave(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("postFileSave\n");
}

//----------------------------------------------------------------------------------------------------------------------
void Global::onPluginLoad()
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("Registering callbacks\n");
  m_fileNew = MSceneMessage::addCallback(MSceneMessage::kAfterNew, onFileNew);
  m_preSave = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, preFileSave);
  m_postSave = MSceneMessage::addCallback(MSceneMessage::kAfterSave, postFileSave);
  m_preOpen = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, preFileOpen);
  m_postOpen = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, postFileOpen);

  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("Registering USD plugins\n");
  // Let USD know about the additional plugins
  std::string pluginLocation(TfStringCatPaths(TfGetenv(AL_USDMAYA_LOCATION_NAME), "share/usd/plugins"));
  PlugRegistry::GetInstance().RegisterPlugins(pluginLocation);

  // For callback initialization for stage cache callback, it will be done via proxy node attribute change.
}

//----------------------------------------------------------------------------------------------------------------------
void Global::onPluginUnload()
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("Removing callbacks\n");
  MSceneMessage::removeCallback(m_fileNew);
  MSceneMessage::removeCallback(m_preSave);
  MSceneMessage::removeCallback(m_postSave);
  MSceneMessage::removeCallback(m_preOpen);
  MSceneMessage::removeCallback(m_postOpen);
  StageCache::removeCallbacks();
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // al
//----------------------------------------------------------------------------------------------------------------------

