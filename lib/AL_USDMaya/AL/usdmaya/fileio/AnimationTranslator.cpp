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
#include <algorithm>
#include <iterator>

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

static MString INHERITS_TRANSFORM_ATTR {"inheritsTransform"};

const std::array<MFn::Type, 4> AnimationTranslator::s_nodeTypesConsiderToBeAnimation {
    MFn::kAnimCurveTimeToAngular,  //79
    MFn::kAnimCurveTimeToDistance,  //80
    MFn::kAnimCurveTimeToTime,   //81
    MFn::kAnimCurveTimeToUnitless  //82
};

//----------------------------------------------------------------------------------------------------------------------
template<class Container, class ValueType >
bool _contains(const Container &container, const ValueType &value)
{
  auto end = container.cend();
  auto first = std::lower_bound(container.cbegin(), end, value);
  return first != end && !(value < *first);
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::considerToBeAnimation(const MFn::Type nodeType)
{
  return _contains<std::array<MFn::Type, 4>, MFn::Type>(s_nodeTypesConsiderToBeAnimation, nodeType);
}

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
    // or we have some special source attributes.
    int32_t i = 0, n = plugs.length();
    for(; i < n; ++i)
    {
      // Test if we have animCurves:
      MObject connectedNode = plugs[i].node();
      const MFn::Type connectedNodeType = connectedNode.apiType();
      if(considerToBeAnimation(connectedNodeType))
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
bool AnimationTranslator::inheritTransform(const MDagPath &path)
{
  MStatus status;
  const MObject transformNode = path.node();
  if(!status)
    return false;

  MPlug inheritTransformPlug (transformNode, AnimationCheckTransformAttributes::getInstance()->inheritTransformAttribute());
  return inheritTransformPlug.asBool();
}

bool AnimationTranslator::areTransformAttributesConnected(const MDagPath &path)
{
  MStatus status;
  const MObject transformNode = path.node(&status);
  if(!status)
    return false;

  int index = 0;
  for(const auto& attributeObject: *AnimationCheckTransformAttributes::getInstance())
  {
    const MPlug plug(transformNode, attributeObject.object());
    if(plug.isDestination(&status))
      return true;

    index++;
  }
  return false;
}

bool AnimationTranslator::isAnimatedTransform(const MObject& transformNode)
{
  if(!transformNode.hasFn(MFn::kTransform))
    return false;

  MStatus status;
  MFnDagNode fnNode(transformNode, &status);
  if (!status)
    return false;

  MDagPath currPath;
  fnNode.getPath(currPath);

  // For better testing:
  AnimationCheckTransformAttributes *attrs = AnimationCheckTransformAttributes::getInstance();
  if(!attrs->isInitialised())
    attrs->initialise(transformNode);

  bool transformAttributeConnected = areTransformAttributesConnected(currPath);
  if(!inheritTransform(currPath) && !transformAttributeConnected)
    return false;

  if(transformAttributeConnected)
    return true;


  while(currPath.pop() == MStatus::kSuccess && inheritTransform(currPath))
  {
    if(areTransformAttributesConnected(currPath))
      return true;
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
AnimationCheckTransformAttributes *AnimationCheckTransformAttributes::s_instance = NULL;

AnimationCheckTransformAttributes *AnimationCheckTransformAttributes::getInstance()
{
  if(!s_instance)
    s_instance = new AnimationCheckTransformAttributes();

  return s_instance;
}

void AnimationCheckTransformAttributes::destruct()
{
  if(s_instance)
  {
    delete s_instance;
    s_instance = NULL;
  }
}

#define _initAttribute(index, attributeName) \
  plug = fn.findPlug(attributeName, &status); \
  if(status) { m_commonTransformAttributes[index] = plug.attribute(&status);} \
  if(status != MS::kSuccess) { m_initialised = false; return false; }

bool AnimationCheckTransformAttributes::initialise(const MObject &transformNode)
{
  if(!transformNode.hasFn(MFn::kTransform))
  {
    m_initialised = false;
    return false;
  }

  MStatus status;
  MFnDependencyNode fn(transformNode);
  MPlug plug;

  _initAttribute(0, "translate")
  _initAttribute(1, "translateX")
  _initAttribute(2, "translateY")
  _initAttribute(3, "translateZ")
  _initAttribute(4, "rotate")
  _initAttribute(5, "rotateX")
  _initAttribute(6, "rotateY")
  _initAttribute(7, "rotateZ")
  _initAttribute(8, "scale")
  _initAttribute(9, "scaleX")
  _initAttribute(10, "scaleY")
  _initAttribute(11, "scaleZ")
  _initAttribute(12, "rotateOrder")

  plug = fn.findPlug("inheritsTransform", &status);
  if(status)
  {
    m_inheritTransformAttribute = plug.attribute(&status);
  }
  if(status != MS::kSuccess)
  {
    m_initialised = false;
    return false;
  }

  m_initialised = true;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
