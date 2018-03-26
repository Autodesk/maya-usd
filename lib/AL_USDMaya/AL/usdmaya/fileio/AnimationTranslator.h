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
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"

#include "maya/MPlug.h"
#include "maya/MString.h"
#include "maya/MDagPath.h"
#include "maya/MObjectHandle.h"

#include <vector>
#include <array>
#include <map>

#include <utility>

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

/// \brief  operator to compare MPlugs with < operator
struct compare_MPlug
{
  bool operator () (const MPlug& a, const MPlug& b) const
  {
    return strcmp(a.name().asChar(), b.name().asChar()) < 0;;
  }
};

/// \brief  operator to compare MDagPaths with < operator
struct compare_MDagPath
{
  bool operator () (const MDagPath& a, const MDagPath& b) const
  {
    return strcmp(a.fullPathName().asChar(), b.fullPathName().asChar()) < 0;;
  }
};

namespace AL {
namespace usdmaya {
namespace fileio {

/// \brief  an attribute that has a scaling on it (due to unit differences)
struct ScaledPair
{
  UsdAttribute attr; ///< the attribute to export
  float scale; ///< the scale to apply
};

typedef std::map<MPlug, UsdAttribute, compare_MPlug> PlugAttrVector;
typedef std::map<MDagPath, UsdAttribute, compare_MDagPath> MeshAttrVector;
typedef std::map<MPlug, ScaledPair, compare_MPlug> PlugAttrScaledVector;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A utility class to help with exporting animated plugs from maya
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct AnimationTranslator
{
  /// \brief  returns true if the attribute is animated
  /// \param  node the node which contains the attribute to test
  /// \param  attr the attribute handle
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  /// \return true if the attribute was found to be animated
  /// This test only covers the situation that your attribute is actually animated by some types of nodes, e.g. animCurves
  /// or expression, or source attribute's full-name match a certain string.
  /// But in reality the control network might be really complicated and heavily customized thus it might go far beyond the
  /// situation we can cover here.
  static bool isAnimated(const MObject& node, const MObject& attr, const bool assumeExpressionIsAnimated = true)
    { return isAnimated(MPlug(node, attr), assumeExpressionIsAnimated); }

  /// \brief  returns true if the attribute is animated
  /// \param  attr the attribute to test
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  /// \return true if the attribute was found to be animated
  /// This test only covers the situation that your attribute is actually animated by some types of nodes, e.g. animCurves
  /// or expression, or source attribute's full-name match a certain string.
  /// But in reality the control network might be really complicated and heavily customized thus it might go far beyond the
  /// situations we can cover here.
  static bool isAnimated(MPlug attr, bool assumeExpressionIsAnimated = true);

  /// \brief  returns true if the mesh is animated
  /// \param  mesh the mesh to test
  /// \return true if the mesh was found to be animated
  /// This test only covers the situation that your node / upstream nodes are actually animated by animCurves.
  /// But in reality the control network might be really complicated and heavily customized thus it might go far beyond the
  /// situations we can cover here.
  static bool isAnimatedMesh(const MDagPath& mesh);

  /// \brief  returns true if the transform node is animated
  /// \param  transformNode the transform node to test
  /// \return true if the transform was found to be animated
  /// It roughly tests a list of common transform attributes, translate, rotate, rotateOrder and scale,
  /// if any of those attributes is connected as destination, we take the transform node as animated.
  /// This test will be performed recursively up to parent hierarchies, unless the inheritsTransform
  /// attribute is turned off.
  static bool isAnimatedTransform(const MObject& transformNode);

  /// \brief  add a plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  /// \param  assumIsAnimated It is a indicator that we already did the isAnimated() check beforehand and we do not want to do it again.
  inline void addPlug(const MPlug& plug,
                      const UsdAttribute& attribute,
                      const bool assumeExpressionIsAnimated,
                      const bool assumIsAnimated = false)
  {
    if(m_animatedPlugs.find(plug) != m_animatedPlugs.end())
      return;

    // assumIsAnimated: isAnimated() check is already done, we don't not want to perform it again:
    if(assumIsAnimated || isAnimated(plug, assumeExpressionIsAnimated))
      m_animatedPlugs.emplace(plug, attribute);
  }

  /// \brief  add a plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  /// \param  scale a scale to apply to convert units if needed
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  /// \param  assumIsAnimated It is a indicator that we already did the isAnimated() check beforehand and we do not want to do it again.
  inline void addPlug(const MPlug& plug,
                      const UsdAttribute& attribute,
                      const float scale,
                      const bool assumeExpressionIsAnimated,
                      const bool assumIsAnimated = false)
  {
    if(m_scaledAnimatedPlugs.find(plug) != m_scaledAnimatedPlugs.end())
      return;

    // assumIsAnimated: isAnimated() check is already done, we don't not want to perform it again:
    if(assumIsAnimated || isAnimated(plug, assumeExpressionIsAnimated))
      m_scaledAnimatedPlugs.emplace(plug, ScaledPair{attribute, scale});
  }

  /// \brief  add a transform plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  ///         attribute can't be handled by generic DgNodeTranslator
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  inline void addTransformPlug(const MPlug& plug, const UsdAttribute& attribute, const bool assumeExpressionIsAnimated)
  {
    if(m_animatedTransformPlugs.find(plug) != m_animatedTransformPlugs.end())
      return;
    if (isAnimated(plug, assumeExpressionIsAnimated))
      m_animatedTransformPlugs.emplace(plug, attribute);
  }

  /// \brief  add a transform plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  ///         attribute can't be handled by generic DgNodeTranslator
  inline void forceAddTransformPlug(const MPlug& plug, const UsdAttribute& attribute)
  {
    if(m_animatedTransformPlugs.find(plug) == m_animatedTransformPlugs.end())
      m_animatedTransformPlugs.emplace(plug, attribute);
  }

  /// \brief  add a scaled plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  ///         attribute can't be handled by generic DgNodeTranslator
  /// \param  scale a scale to apply to convert units if needed
  inline void forceAddPlug(const MPlug& plug, const UsdAttribute& attribute, const float scale)
  {
    if(m_scaledAnimatedPlugs.find(plug) == m_scaledAnimatedPlugs.end())
      m_scaledAnimatedPlugs.emplace(plug, ScaledPair{ attribute, scale });
  }

  /// \brief  add an animated plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  ///         attribute can't be handled by generic DgNodeTranslator
  inline void forceAddPlug(const MPlug& plug, const UsdAttribute& attribute)
  {
    if(m_animatedPlugs.find(plug) == m_animatedPlugs.end())
      m_animatedPlugs.emplace(plug, attribute);
  }

  /// \brief  add a mesh to the animation translator
  /// \param  path the path to the animated maya mesh
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  inline void addMesh(const MDagPath& path, const UsdAttribute& attribute)
  {
    if(m_animatedMeshes.find(path) == m_animatedMeshes.end())
      m_animatedMeshes.emplace(path, attribute);
  }

  /// \brief  After the scene has been exported, call this method to export the animation data on various attributes
  /// \param  params the export options
  void exportAnimation(const ExporterParams& params);
private:
  static bool considerToBeAnimation(const MFn::Type nodeType);
  static bool inheritTransform(const MDagPath &path);
  static bool areTransformAttributesConnected(const MDagPath &path);
private:
  PlugAttrVector m_animatedPlugs;
  PlugAttrScaledVector m_scaledAnimatedPlugs;
  PlugAttrVector m_animatedTransformPlugs;
  MeshAttrVector m_animatedMeshes;

  const static std::array<MFn::Type, 4> s_nodeTypesConsiderToBeAnimation;
};


class AnimationCheckTransformAttributes
{
#define ATTR_COUNT 13
public:
  AnimationCheckTransformAttributes();
  inline std::array<MObjectHandle, ATTR_COUNT>::iterator begin() {return m_commonTransformAttributes.begin();}
  inline std::array<MObjectHandle, ATTR_COUNT>::iterator end() {return m_commonTransformAttributes.end();}
  inline MObject inheritTransformAttribute()const {return m_inheritTransformAttribute.object();}

private:
  std::array<MObjectHandle, ATTR_COUNT> m_commonTransformAttributes;
  MObjectHandle m_inheritTransformAttribute;
};

extern AnimationCheckTransformAttributes g_AnimationCheckTransformAttributes;

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
