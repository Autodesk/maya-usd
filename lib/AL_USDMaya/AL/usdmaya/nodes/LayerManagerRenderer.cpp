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

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::onRendererChanged()
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
void LayerManager::changeRendererPlugin(ProxyShape* proxy, bool creation)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::changeRendererPlugin\n");
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
int LayerManager::getRendererPluginIndex() const
{
  MPlug plug(thisMObject(), m_rendererPluginName);
  MString pluginName = plug.asString();
  return m_rendererPluginsNames.indexOf(pluginName);
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::setRendererPlugin(const MString& pluginName)
{
  int index = m_rendererPluginsNames.indexOf(pluginName);
  if (index >= 0)
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("LayerManager::setRendererPlugin\n");
    MPlug plug(thisMObject(), m_rendererPluginName);
    plug.setString(pluginName);
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
