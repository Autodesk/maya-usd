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
static const char* const eventTypeStrings[] =
{
  "custom",
  "schema",
  "coremaya",
  "usdmaya"
};

//----------------------------------------------------------------------------------------------------------------------
class MayaEventSystemBinding
  : public maya::EventSystemBinding
{
public:

  MayaEventSystemBinding()
    : EventSystemBinding(eventTypeStrings, sizeof(eventTypeStrings) / sizeof(const char*)) {}

  bool executePython(const char* const code) override
  {
    return MGlobal::executePythonCommand(code, false, true);
  }

  bool executeMEL(const char* const code) override
  {
    return MGlobal::executeCommand(code, false, true);
  }

  void writeLog(EventSystemBinding::Type severity, const char* const text) override
  {
    switch(severity)
    {
    case kInfo: MGlobal::displayInfo(text); break;
    case kWarning: MGlobal::displayWarning(text); break;
    case kError: MGlobal::displayError(text); break;
    }
  }
};

static MayaEventSystemBinding g_eventSystem;

//----------------------------------------------------------------------------------------------------------------------
maya::CallbackId Global::m_preSave;
maya::CallbackId Global::m_postSave;
maya::CallbackId Global::m_preOpen;
maya::CallbackId Global::m_postOpen;
maya::CallbackId Global::m_fileNew;

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

  MFnDependencyNode fn;
  {
    MItDependencyNodes iter(MFn::kPluginShape);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == nodes::ProxyShape::kTypeId)
      {
        nodes::ProxyShape* proxy = (nodes::ProxyShape*)fn.userNode();
        proxy->removeAttributeChangedCallback();
      }
    }
  }
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
        nodes::ProxyShape* proxy = (nodes::ProxyShape*)fn.userNode();
        proxy->reloadStage();
        proxy->constructGLImagingEngine();
        auto stage = proxy->getUsdStage();
        proxy->deserialiseTranslatorContext();
        proxy->findTaggedPrims();
        proxy->deserialiseTransformRefs();
        auto layer = proxy->getLayer();
        if(layer)
        {
          layer->setLayerAndClearAttribute(stage->GetSessionLayer());
        }
        proxy->addAttributeChangedCallback();
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

  maya::EventScheduler::initScheduler(&g_eventSystem);
  auto ptr = new maya::MayaEventHandler(&maya::EventScheduler::getScheduler(), maya::kMayaEventType);
  new maya::MayaEventManager(ptr);

  auto& manager = maya::MayaEventManager::instance();
  m_fileNew = manager.registerCallback(onFileNew, "AfterNew", "usdmaya_onFileNew", 0x1000);
  m_preSave = manager.registerCallback(preFileSave, "BeforeSave", "usdmaya_preFileSave", 0x1000);
  m_postSave = manager.registerCallback(postFileSave, "AfterSave", "usdmaya_postFileSave", 0x1000);
  m_preOpen = manager.registerCallback(preFileOpen, "BeforeOpen", "usdmaya_preFileOpen", 0x1000);
  m_postOpen = manager.registerCallback(postFileOpen, "AfterOpen", "usdmaya_postFileOpen", 0x1000);

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
  auto& manager = maya::MayaEventManager::instance();
  manager.unregisterCallback(m_fileNew);
  manager.unregisterCallback(m_preSave);
  manager.unregisterCallback(m_postSave);
  manager.unregisterCallback(m_preOpen);
  manager.unregisterCallback(m_postOpen);
  StageCache::removeCallbacks();

  maya::MayaEventManager::freeInstance();
  maya::EventScheduler::freeScheduler();
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // al
//----------------------------------------------------------------------------------------------------------------------

