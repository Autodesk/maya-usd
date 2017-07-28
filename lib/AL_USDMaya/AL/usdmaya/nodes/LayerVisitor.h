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
#pragma once
#include "AL/usdmaya/Common.h"

#include <stack>
#include <vector>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that follows the visitor pattern to walk through all layer nodes associated with the specified
///         proxy shape node.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class LayerVisitor
{
public:

  /// \brief  ctor
  /// \param  shape the proxy shape whose layers you wish to iterate over
  LayerVisitor(ProxyShape* shape);

  /// \brief  dtor
  virtual ~LayerVisitor() {}

  /// \brief  call to visit all of the layers
  void visitAll();

  /// \brief  override to implement your own custom processing for each layer.
  virtual void onVisit() {}

  /// \brief  returns the current layer being visited
  /// \return the current layer
  inline Layer* thisLayer() const
    { return m_stack.top().m_thisLayer; }

  /// \brief  returns the parent of the current layer being visited
  /// \return the parent of the current layer
  inline Layer* parentLayer() const
    { return m_stack.top().m_parentLayer; }

  /// \brief  returns true if the current layer is a sub layer
  /// \return true if the current layer is a sub layer, false if it's a child layer
  inline bool isSubLayer() const
    { return m_stack.top().m_isSubLayer; }

  /// \brief  returns the recursion depth.
  /// \return the recursion depth
  inline uint32_t depth() const
    { return static_cast<uint32_t>(m_stack.size()); }

private:
  void visit();
  struct StackItem
  {
    StackItem(Layer* layer, const bool isSubLayer);
    std::vector<Layer*> m_subLayers;
    std::vector<Layer*> m_childLayers;
    Layer* m_thisLayer;
    Layer* m_parentLayer;
    int32_t m_index;
    bool m_isSubLayer;
  };

  std::stack<StackItem> m_stack;
  ProxyShape* m_shape;
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
