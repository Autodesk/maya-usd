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
#include "AL/usdmaya/nodes/LayerVisitor.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
LayerVisitor::StackItem::StackItem(Layer* layer, const bool isSubLayer)
  : m_subLayers(layer->getSubLayers()),
    m_childLayers(layer->getChildLayers()),
    m_thisLayer(layer),
    m_parentLayer(layer->getParentLayer()),
    m_index(-1),
    m_isSubLayer(isSubLayer)
{
}

//----------------------------------------------------------------------------------------------------------------------
LayerVisitor::LayerVisitor(ProxyShape* shape) : m_stack(), m_shape(shape)
{
  Layer* layer = shape->getLayer();
  if(layer)
  {
    m_stack.push(StackItem(layer, false));
  }
}


//----------------------------------------------------------------------------------------------------------------------
void LayerVisitor::visitAll()
{
  while(!m_stack.empty())
    visit();
}

//----------------------------------------------------------------------------------------------------------------------
void LayerVisitor::visit()
{
  this->onVisit();
  {
    StackItem& si = m_stack.top();
    ++si.m_index;
  }

  while(!m_stack.empty())
  {
    StackItem& si = m_stack.top();
    const size_t numSubLayers = si.m_subLayers.size();
    const size_t numChildLayers = si.m_childLayers.size();
    const size_t numTotalLayers = numSubLayers + numChildLayers;

    // if we can walk down a sub layer
    if(numSubLayers > si.m_index)
    {
      m_stack.push(StackItem(si.m_subLayers[si.m_index], true));
      return;
    }
    else
    // if we can walk down a child layer
    if(numTotalLayers > si.m_index)
    {
      m_stack.push(StackItem(si.m_childLayers[si.m_index - numSubLayers], false));
      return;
    }
    else
    // if we have done iterating at this level, pop
    {
      m_stack.pop();
      if(!m_stack.empty())
      {
        StackItem& si = m_stack.top();
        ++si.m_index;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

