//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
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

#include "AL/maya/utils/NodeHelper.h"
#include "AL/maya/utils/MayaHelperMacros.h"

#include "pxr/usd/usd/stage.h"

#include <maya/MPxNode.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectHandle.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The MeshAnimCreator node acts as a polyCreator node within the DG that is driven by time. When the time
///         changes, 
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class MeshAnimCreator
  : public MPxNode,
    public AL::maya::utils::NodeHelper
{
public:

  /// \brief  ctor
  inline MeshAnimCreator()
    : MPxNode(), NodeHelper()
     {}

  inline ~MeshAnimCreator()
    { MNodeMessage::removeCallback(m_attributeChanged); }

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  //--------------------------------------------------------------------------------------------------------------------
  /// Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------

  AL_DECL_ATTRIBUTE(primPath);
  AL_DECL_ATTRIBUTE(inTime);
  AL_DECL_ATTRIBUTE(inStageData);
  AL_DECL_ATTRIBUTE(outMesh);

private:
  void postConstructor() override;
  MStatus connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
  MStatus connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc) override;
  static void onAttributeChanged(MNodeMessage::AttributeMessage, MPlug&, MPlug&, void*);
  MStatus compute(const MPlug& plug, MDataBlock& data) override;
  UsdStageRefPtr getStage();
private:
  SdfPath m_cachePath;
  MObjectHandle proxyShapeHandle;
  MCallbackId m_attributeChanged = 0;
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
