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
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/stackTrace.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include "maya/MGlobal.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MItDependencyNodes.h"

#include <iostream>

#ifndef AL_USDMAYA_LOCATION_NAME
  #define AL_USDMAYA_LOCATION_NAME "AL_USDMAYA_LOCATION"
#endif

namespace {
  // Keep track of how many levels "deep" in file reads we are - because
  // a file open can can trigger a reference load, which can trigger a
  // a sub-reference load, etc... we only want to run postFileRead after
  // once per top-level file read operation (ie, once per open, or once
  // per import, or once per reference).
  std::atomic<size_t> readDepth;
}

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
  : public AL::event::EventSystemBinding
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
AL::event::CallbackId Global::m_preSave;
AL::event::CallbackId Global::m_postSave;
AL::event::CallbackId Global::m_preRead;
AL::event::CallbackId Global::m_postRead;
AL::event::CallbackId Global::m_fileNew;

//----------------------------------------------------------------------------------------------------------------------
static void onFileNew(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("onFileNew\n");
  // These should both clear the caches, however they don't actually do anything of the sort. Puzzled.
  UsdUtilsStageCache::Get().Clear();
  StageCache::Clear();
}

//----------------------------------------------------------------------------------------------------------------------
static void preFileRead(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("preFileRead\n");

  if(!readDepth)
  {
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

  ++readDepth;
}

//----------------------------------------------------------------------------------------------------------------------
static void disableAttributeChangedCallbacks()
{
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
        proxy->removeAttributeChangedCallback();
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
static void postFileRead(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("postFileRead\n");

  // If the plugin is loaded as the result of a "requires" statment when opening a scene,
  // it's possible for postFileRead to be called without preFileRead having been... so,
  // make sure we don't decrement past 0!
  size_t oldReadDepth = readDepth.fetch_sub(1);
  if (ARCH_UNLIKELY(oldReadDepth == 0))
  {
    // Assume we got here because we didn't call preFileRead - do the increment that it
    // missed
    readDepth++;
    oldReadDepth++;
    disableAttributeChangedCallbacks();
  }

  // oldReadDepth is the value BEFORE we decremented (with fetch_sub), so should be 1
  // if we're now "done"
  if (oldReadDepth != 1)
  {
    return;
  }

  nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
  if (layerManager)
  {
    layerManager->loadAllLayers();
    AL_MAYA_CHECK_ERROR2(layerManager->clearSerialisationAttributes(), "postFileRead");
  }

  MFnDependencyNode fn;
  {
    std::vector<MObjectHandle>& unloadedProxies = nodes::ProxyShape::GetUnloadedProxyShapes();
    unsigned int numUnloadedProxies = unloadedProxies.size();
    for(unsigned int i = 0; i < numUnloadedProxies; ++i)
    {
      if(!(unloadedProxies[i].isValid() && unloadedProxies[i].isAlive()))
      {
        continue;
      }
      fn.setObject(unloadedProxies[i].object());
      if(fn.typeId() != nodes::ProxyShape::kTypeId)
      {
        TF_CODING_ERROR("ProxyShape::m_unloadedProxyShapes had a non-Proxy-Shape mobject");
        continue;
      }

      // execute a pull on each proxy shape to ensure that each one has a valid USD stage!
      nodes::ProxyShape* proxy = (nodes::ProxyShape*)fn.userNode();
      proxy->loadStage();
      auto stage = proxy->getUsdStage();
      proxy->deserialiseTranslatorContext();
      proxy->findTaggedPrims();
      proxy->deserialiseTransformRefs();
      proxy->constructGLImagingEngine();
      proxy->addAttributeChangedCallback();
    }
    unloadedProxies.clear();
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
static void _preFileSave()
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("preFileSave\n");

  // currently, if we have selected a shape in the usd proxy shape, a series of transforms will have been created.
  // Ideally we don't want these transient nodes to be stored in the Maya file, so make sure we unselect prior to a file
  // save (which should call another set of callbacks and delete those transient nodes. This should leave us with just
  // those AL::usdmaya::nodes::Transform nodes that are created because they are required, or have been requested).
  MGlobal::clearSelectionList();

  nodes::ProxyShape::serializeAll();
}

//----------------------------------------------------------------------------------------------------------------------
static void preFileSave(void*)
{
  // This is a file-save callback, so we want to be EXTRA careful not to crash out, and
  // lose their work right when they need it most!
  // ...except if we're in a debug build, in which case just crash the mofo, so we
  // notice!
#ifndef DEBUG
  try
  {
#endif
    _preFileSave();
#ifndef DEBUG
  }
  catch (const std::exception& e)
  {
    MString msg("Caught unhandled exception inside of al_usdmaya save callback: ");
    msg += e.what();
    MGlobal::displayError(msg);
    std::cerr << msg.asChar();
    TfPrintStackTrace(std::cerr, "Unhandled error in al_usdmaya save callback:");
  }
  catch (...)
  {
    MGlobal::displayError("Caught unknown exception inside of al_usdmaya save callback");
    TfPrintStackTrace(std::cerr, "Unknown error in al_usdmaya save callback:");
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
static void postFileSave(void*)
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("postFileSave\n");

  nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
  if (layerManager)
  {
    AL_MAYA_CHECK_ERROR2(layerManager->clearSerialisationAttributes(), "postFileSave");
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Global::onPluginLoad()
{
  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("Registering callbacks\n");

  AL::event::EventScheduler::initScheduler(&g_eventSystem);
  auto ptr = new AL::maya::event::MayaEventHandler(&AL::event::EventScheduler::getScheduler(), AL::event::kMayaEventType);
  new AL::maya::event::MayaEventManager(ptr);

  auto& manager = AL::maya::event::MayaEventManager::instance();
  m_fileNew = manager.registerCallback(onFileNew, "AfterNew", "usdmaya_onFileNew", 0x1000);
  m_preSave = manager.registerCallback(preFileSave, "BeforeSave", "usdmaya_preFileSave", 0x1000);
  m_postSave = manager.registerCallback(postFileSave, "AfterSave", "usdmaya_postFileSave", 0x1000);
  m_preRead = manager.registerCallback(preFileRead, "BeforeFileRead", "usdmaya_preFileRead", 0x1000);
  m_postRead = manager.registerCallback(postFileRead, "AfterFileRead", "usdmaya_postFileRead", 0x1000);

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
  auto& manager = AL::maya::event::MayaEventManager::instance();
  manager.unregisterCallback(m_fileNew);
  manager.unregisterCallback(m_preSave);
  manager.unregisterCallback(m_postSave);
  manager.unregisterCallback(m_preRead);
  manager.unregisterCallback(m_postRead);
  StageCache::removeCallbacks();

  AL::maya::event::MayaEventManager::freeInstance();
  AL::event::EventScheduler::freeScheduler();
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // al
//----------------------------------------------------------------------------------------------------------------------

