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

#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/MeshTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"

#include "maya/MItDependencyGraph.h"
#include "maya/MFnAnimCurve.h"
#include "maya/MAnimControl.h"
#include "maya/MGlobal.h"
#include "maya/MFnMesh.h"
#include "maya/MAnimUtil.h"

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::isAnimated(MPlug attr, const bool assumeExpressionIsAnimated)
{
  if(attr.isArray())
  {
    const uint32_t numElements = attr.numElements();
    for(uint32_t i = 0; i < numElements; ++i)
    {
      if(isAnimated(attr.elementByLogicalIndex(i), assumeExpressionIsAnimated))
      {
        return true;
      }
    }
    return false;
  }

  if(attr.isCompound())
  {
    const uint32_t numChildren = attr.numChildren();
    for(uint32_t i = 0; i < numChildren; ++i)
    {
      if(isAnimated(attr.child(i), assumeExpressionIsAnimated))
      {
        return true;
      }
    }
  }

  // is no connections exist, it cannot be animated
  if(!attr.isConnected())
  {
    return false;
  }
  else
  {
    MPlugArray plugs;
    if(!attr.connectedTo(plugs, true, false))
    {
      return false;
    }

    // test to see if we are directly connected to an animation curve
    int32_t i = 0, n = plugs.length();
    for(; i < n; ++i)
    {
      MObject connectedNode = plugs[i].node();
      const MFn::Type connectedNodeType = connectedNode.apiType();
      if(connectedNodeType == MFn::kAnimCurveTimeToAngular ||
         connectedNodeType == MFn::kAnimCurveTimeToDistance ||
         connectedNodeType == MFn::kAnimCurveTimeToTime ||
         connectedNodeType == MFn::kAnimCurveTimeToUnitless)
      {
        // I could use some slightly better heuristics here.
        // If there are 2 or more keyframes on this curve, assume its value changes.
        MFnAnimCurve curve(connectedNode);
        if(curve.numKeys() > 1)
        {
          return true;
        }
      }
      else
      {
        break;
      }
    }

    // if all connected nodes are anim curves, and all have 1 or zero keys, the plug is not animated.
    if(i == n)
    {
      return false;
    }
  }

  // if we get here, recurse through the upstream connections looking for a time or expression node
  MStatus status;
  MItDependencyGraph iter(
    attr,
    MFn::kInvalid,
    MItDependencyGraph::kUpstream,
    MItDependencyGraph::kDepthFirst,
    MItDependencyGraph::kNodeLevel,
    &status);

  if (!status)
  {
    MGlobal::displayError("Unable to create DG iterator");
    return false;
  }

  for(; !iter.isDone(); iter.next())
  {
    MObject currNode = iter.thisPlug().node();
    if(currNode.hasFn(MFn::kTime))
    {
      return true;
    }
    if(assumeExpressionIsAnimated && currNode.hasFn(MFn::kExpression))
    {
      return true;
    }
    if ((currNode.hasFn(MFn::kTransform) || currNode.hasFn(MFn::kPluginTransformNode)) &&
        MAnimUtil::isAnimated(currNode, true))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::isAnimatedMesh(const MDagPath& mesh)
{
  if (MAnimUtil::isAnimated(mesh, true))
  {
    return true;
  }
  MStatus status;
  MObject node = mesh.node();
  MItDependencyGraph iter(
    node,
    MFn::kInvalid,
    MItDependencyGraph::kUpstream,
    MItDependencyGraph::kDepthFirst,
    MItDependencyGraph::kNodeLevel,
    &status);
  if (!status)
  {
    MGlobal::displayError("Unable to create DG iterator");
    return false;
  }
  for (; !iter.isDone(); iter.next())
  {
    MObject currNode = iter.thisPlug().node();
    if ((currNode.hasFn(MFn::kTransform) || currNode.hasFn(MFn::kPluginTransformNode))
        && MAnimUtil::isAnimated(currNode, true))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void AnimationTranslator::exportAnimation(const ExporterParams& params)
{
  auto const startAttrib =  m_animatedPlugs.begin();
  auto const endAttrib =  m_animatedPlugs.end();
  auto const startAttribScaled =  m_scaledAnimatedPlugs.begin();
  auto const endAttribScaled =  m_scaledAnimatedPlugs.end();
  auto const startTransformAttrib = m_animatedTransformPlugs.begin();
  auto const endTransformAttrib = m_animatedTransformPlugs.end();
  auto const startMesh = m_animatedMeshes.begin();
  auto const endMesh = m_animatedMeshes.end();
  if((startAttrib != endAttrib) ||
     (startAttribScaled != endAttribScaled) ||
     (startTransformAttrib != endTransformAttrib) ||
     (startMesh != endMesh))
  {
    for(double t = params.m_minFrame, e = params.m_maxFrame + 1e-3f; t < e; t += 1.0)
    {
      MAnimControl::setCurrentTime(t);
      UsdTimeCode timeCode(t);
      for(auto it = startAttrib; it != endAttrib; ++it)
      {
        /// \todo This feels wrong. Split the DgNodeTranslator class into 3 ...
        ///         maya::Dg
        ///         usdmaya::Dg
        ///         usdmaya::fileio::translator::Dg
        translators::DgNodeTranslator::copyAttributeValue(it->first, it->second, timeCode);
      }
      for(auto it = startAttribScaled; it != endAttribScaled; ++it)
      {
        /// \todo This feels wrong. Split the DgNodeTranslator class into 3 ...
        ///         maya::Dg
        ///         usdmaya::Dg
        ///         usdmaya::fileio::translator::Dg
        translators::DgNodeTranslator::copyAttributeValue(it->first, it->second.attr, it->second.scale, timeCode);
      }
      for (auto it = startTransformAttrib; it != endTransformAttrib; ++it)
      {
        translators::TransformTranslator::copyAttributeValue(it->first, it->second, timeCode);
      }
      for(auto it = startMesh; it != endMesh; ++it)
      {
        AL::usdmaya::utils::copyVertexData(MFnMesh(it->first), it->second, timeCode);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
