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
#include "AL/maya/NodeHelper.h"
#include "AL/usdmaya/DrivenTransformsData.h"

#include "maya/MPxNode.h"
#include "maya/MPlugArray.h"
#include "maya/MEvaluationNode.h"

#include <memory>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The AL::usdmaya::nodes::HostDrivenTransforms relays Maya animated transform data including translate, scale,
///         rotate and visibility to USD prims. It works by plugging inputs onto these "driven" attributes and outDrivenTransformsData
///         to inDrivenTransformsData of AL::usdmaya::node::ProxyShape. AL::usdmaya::node::ProxyShape is in charge of computing
///         and pushing combined transform matrix into USD prims assigned here by drivenPrimPaths.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class HostDrivenTransforms
  : public MPxNode
  , public maya::NodeHelper
{
public:

  /// \brief ctor
  HostDrivenTransforms();

  /// \brief dtor
  ~HostDrivenTransforms();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Type Info & Registration
  /// \todo needs documenting!
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  /// an array of strings that represent the paths to be driven
  AL_DECL_ATTRIBUTE(drivenPrimPaths);

  /// an array of rotation values for the driven transforms
  AL_DECL_ATTRIBUTE(drivenRotate);

  /// an array of rotate order values for the driven transforms
  AL_DECL_ATTRIBUTE(drivenRotateOrder);

  /// an array of scale values for the driven transforms
  AL_DECL_ATTRIBUTE(drivenScale);

  /// an array of translation values for the driven transforms
  AL_DECL_ATTRIBUTE(drivenTranslate);

  /// an array of visibility flags for the driven transforms
  AL_DECL_ATTRIBUTE(drivenVisibility);

  /// a plug-in data to convey driven transforms
  AL_DECL_ATTRIBUTE(outDrivenTransformsData);

private:
  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Virtual overrides
  //--------------------------------------------------------------------------------------------------------------------
  MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
  bool getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx) override;
  bool setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx) override;
  MPxNode::SchedulingType schedulingType() const override { return kParallel; }

  void updatePrimPaths(DrivenTransforms& drivenTransforms);
  void updateMatrix(MDataBlock& dataBlock, DrivenTransforms& drivenTransforms);
  void updateVisibility(MDataBlock& dataBlock, DrivenTransforms& drivenTransforms);

  std::vector<std::string>  m_primPaths;
};
//----------------------------------------------------------------------------------------------------------------------
}// nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
