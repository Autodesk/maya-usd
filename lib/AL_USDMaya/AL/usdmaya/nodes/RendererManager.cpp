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
#include "AL/usdmaya/nodes/RendererManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include "pxr/usdImaging/usdImaging/version.h"
#if (USD_IMAGING_API_VERSION >= 7)
  #include "pxr/usdImaging/usdImagingGL/gl.h"
  #include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#else
  #include "pxr/usdImaging/usdImaging/gl.h"
  #include "pxr/usdImaging/usdImaging/hdEngine.h"
#endif

#include "maya/MGlobal.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MItDependencyNodes.h"

namespace {

  static std::mutex s_findNodeMutex;

}

namespace AL {
namespace usdmaya {
namespace nodes {

RendererManager::~RendererManager()
{
  removeAttributeChangedCallback();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(RendererManager, AL_USDMAYA_RENDERERMANAGER, AL_usdmaya);

MObject RendererManager::m_rendererPluginName = MObject::kNullObj;
MObject RendererManager::m_rendererPlugin = MObject::kNullObj;

TfTokenVector RendererManager::m_rendererPluginsTokens;
MStringArray RendererManager::m_rendererPluginsNames;

//----------------------------------------------------------------------------------------------------------------------
MStatus RendererManager::initialise()
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::initialize\n");
  try
  {
    setNodeType(kTypeName);
    addFrame("Renderer plugin");

    // hydra renderer plugin discovery
    // create dummy imaging engine to get renderer names
    UsdImagingGL imagingEngine(SdfPath(), {});
    m_rendererPluginsTokens = imagingEngine.GetRendererPlugins();
    const size_t numTokens = m_rendererPluginsTokens.size();
    m_rendererPluginsNames = MStringArray(numTokens, MString());

    // as it's not clear what is the lifetime of strings returned from HdEngine::GetRendererPluginDesc
    // we store them in array to populate options menu items
    std::vector<std::string> pluginNames(numTokens);
    std::vector<const char*> enumNames(numTokens + 1, nullptr);
    std::vector<int16_t> enumValues(numTokens, -1);
    for (size_t i = 0; i < numTokens; ++i)
    {
      pluginNames[i] = imagingEngine.GetRendererPluginDesc(m_rendererPluginsTokens[i]);
      m_rendererPluginsNames[i] = MString(pluginNames[i].data(), pluginNames[i].size());
      enumNames[i] = pluginNames[i].data();
      enumValues[i] = i;
    }
    
    m_rendererPluginName = addStringAttr(
      "rendererPluginName", "rpn", "GL", kCached | kReadable | kWritable | kStorable | kHidden);
    m_rendererPlugin = addEnumAttr(
      "rendererPlugin", "rp", kInternal | kReadable | kWritable, enumNames.data(), enumValues.data()
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
MObject RendererManager::findNode()
{
  std::lock_guard<std::mutex> lock(s_findNodeMutex);
  return _findNode();
}

//----------------------------------------------------------------------------------------------------------------------
MObject RendererManager::_findNode()
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
MObject RendererManager::findOrCreateNode(MDGModifier* dgmod, bool* wasCreated)
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::findOrCreateNode\n");
  std::lock_guard<std::mutex> lock(s_findNodeMutex);
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
RendererManager* RendererManager::findManager()
{
  MObject manager = findNode();
  if(manager.isNull())
  {
    return nullptr;
  }
  return static_cast<RendererManager*>(MFnDependencyNode(manager).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
RendererManager* RendererManager::findOrCreateManager(MDGModifier* dgmod, bool* wasCreated)
{
  return static_cast<RendererManager*>(MFnDependencyNode(findOrCreateNode(dgmod, wasCreated)).userNode());
}


//----------------------------------------------------------------------------------------------------------------------
void RendererManager::onAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug&, void* clientData)
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::onAttributeChanged\n");
  RendererManager* manager = static_cast<RendererManager*>(clientData);
  assert(manager);
  if(plug == m_rendererPluginName)
    manager->onRendererChanged();
}

//----------------------------------------------------------------------------------------------------------------------
void RendererManager::removeAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::removeAttributeChangedCallback\n");
  if(m_attributeChanged != 0)
  {
    MMessage::removeCallback(m_attributeChanged);
    m_attributeChanged = 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void RendererManager::addAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::addAttributeChangedCallback\n");
  if(m_attributeChanged == 0)
  {
    MObject obj = thisMObject();
    m_attributeChanged = MNodeMessage::addAttributeChangedCallback(obj, onAttributeChanged, (void*)this);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void RendererManager::postConstructor()
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::postConstructor\n");
  addAttributeChangedCallback();
}

//----------------------------------------------------------------------------------------------------------------------
bool RendererManager::setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx)
{
  if (plug == m_rendererPlugin)
  {
    short index = dataHandle.asShort();
    if (index >= 0 && index < m_rendererPluginsNames.length())
    {
      MPlug plug(thisMObject(), m_rendererPluginName);
      plug.setString(m_rendererPluginsNames[index]);
      return true;
    }
  }
  return MPxNode::setInternalValueInContext(plug, dataHandle, ctx);
}

//----------------------------------------------------------------------------------------------------------------------
bool RendererManager::getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx)
{
  if (plug == m_rendererPlugin)
  {
    int index = getRendererPluginIndex();
    if (index >= 0)
    {
      dataHandle.set((short)index);
      return true;
    }
  }
  return MPxNode::getInternalValueInContext(plug, dataHandle, ctx);
}

//----------------------------------------------------------------------------------------------------------------------
void RendererManager::onRendererChanged()
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
      changeRendererPlugin(proxy);
    }
  }
  //! We need to refresh viewport to changes take effect
  MGlobal::executeCommandOnIdle("refresh -force");
}

//----------------------------------------------------------------------------------------------------------------------
void RendererManager::changeRendererPlugin(ProxyShape* proxy, bool creation)
{
  TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::changeRendererPlugin\n");
  assert(proxy);
  if (proxy->engine())
  {
    int rendererId = getRendererPluginIndex();
    if (rendererId >= 0)
    {
      // Skip redundant renderer changes on ProxyShape creation
      if (rendererId == 0 && creation)
        return;
      
      assert(rendererId < m_rendererPluginsTokens.size());
      TfToken plugin = m_rendererPluginsTokens[rendererId];
      if (!proxy->engine()->SetRendererPlugin(plugin))
      {
        MString data(plugin.data());
        MGlobal::displayError(MString("Failed to set renderer plugin: ") + data);
      }
    }
    else
    {
      MPlug plug(thisMObject(), m_rendererPluginName);
      MString pluginName = plug.asString();
      if (pluginName.length())
        MGlobal::displayError(MString("Invalid renderer plugin: ") + pluginName);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
int RendererManager::getRendererPluginIndex() const
{
  MPlug plug(thisMObject(), m_rendererPluginName);
  MString pluginName = plug.asString();
  return m_rendererPluginsNames.indexOf(pluginName);
}

//----------------------------------------------------------------------------------------------------------------------
bool RendererManager::setRendererPlugin(const MString& pluginName)
{
  int index = m_rendererPluginsNames.indexOf(pluginName);
  if (index >= 0)
  {
    TF_DEBUG(ALUSDMAYA_RENDERER).Msg("RendererManager::setRendererPlugin\n");
    MPlug plug(thisMObject(), m_rendererPluginName);
    plug.setString(pluginName);
    return true;
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_RENDERER).Msg("Failed to set renderer plugin!\n");
    MGlobal::displayError(MString("Failed to set renderer plugin: ") + pluginName);
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
