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
#include "maya/MDagPath.h"
#include <vector>
#include <utility>

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

typedef std::pair<MPlug, UsdAttribute> PlugAttrPair;
typedef std::vector<PlugAttrPair> PlugAttrVector;
typedef std::pair<MDagPath, UsdAttribute> MeshAttrPair;
typedef std::vector<MeshAttrPair> MeshAttrVector;
typedef std::pair<PlugAttrPair, float> PlugAttrScaledPair;
typedef std::vector<PlugAttrScaledPair> PlugAttrScaledVector;

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
  static bool isAnimated(const MObject& node, const MObject& attr, const bool assumeExpressionIsAnimated = true)
    { return isAnimated(MPlug(node, attr), assumeExpressionIsAnimated); }

  /// \brief  returns true if the attribute is animated
  /// \param  attr the attribute to test
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  /// \return true if the attribute was found to be animated
  static bool isAnimated(MPlug attr, bool assumeExpressionIsAnimated = true);

  /// \brief  returns true if the mesh is animated
  /// \param  mesh the mesh to test
  /// \return true if the mesh was found to be animated
  static bool isAnimatedMesh(const MDagPath& mesh);

  /// \brief  add a plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  inline void addPlug(const MPlug& plug, const UsdAttribute& attribute, const bool assumeExpressionIsAnimated)
  {
    if(isAnimated(plug, assumeExpressionIsAnimated))
      m_animatedPlugs.emplace_back(std::make_pair(plug, attribute));
  }

  /// \brief  add a plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  /// \param  scale a scale to apply to convert units if needed
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  inline void addPlug(const MPlug& plug, const UsdAttribute& attribute, const float scale, const bool assumeExpressionIsAnimated)
  {
    if(isAnimated(plug, assumeExpressionIsAnimated))
      m_scaledAnimatedPlugs.emplace_back(std::make_pair(std::make_pair(plug, attribute), scale));
  }

  /// \brief  add a transform plug to the animation translator (if the plug is animated)
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  ///         attribute can't be handled by generic DgNodeTranslator
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  inline void addTransformPlug(const MPlug& plug, const UsdAttribute& attribute, const bool assumeExpressionIsAnimated)
  {
    if (isAnimated(plug, assumeExpressionIsAnimated))
      m_animatedTransformPlugs.emplace_back(plug, attribute);
  }

  /// \brief  add a mesh to the animation translator
  /// \param  plug the maya attribute to test
  /// \param  attribute the corresponding maya attribute to write the anim data into if the plug is animated
  /// \param  scale a scale to apply to convert units if needed
  /// \param  assumeExpressionIsAnimated if we encounter an expression, assume that the attribute is animated (true) or
  ///         static (false).
  inline void addMesh(const MDagPath& path, const UsdAttribute& attribute)
  {
    m_animatedMeshes.emplace_back(path, attribute);
  }

  /// \brief  After the scene has been exported, call this method to export the animation data on various attributes
  /// \param  params the export options
  void exportAnimation(const ExporterParams& params);

private:
  PlugAttrVector m_animatedPlugs;
  PlugAttrScaledVector m_scaledAnimatedPlugs;
  PlugAttrVector m_animatedTransformPlugs;
  MeshAttrVector m_animatedMeshes;
};

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
