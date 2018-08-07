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
#include <AL/usdmaya/ForwardDeclares.h>
#include "maya/MSelectionList.h"
#include "maya/MString.h"
#include "AL/usd/utils/ForwardDeclares.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  parameters for the exporter. These parameters are constructed by any command or file translator that wishes
///         to export data from maya, which are then passed to the AL::usdmaya::fileio::Export class to perform the
///         actual export work.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct ExporterParams
{
  MSelectionList m_nodes; ///< the selected node to be exported
  MString m_fileName; ///< the filename of the file we will be exporting
  double m_minFrame=0.0; ///< the start frame for the animation export
  double m_maxFrame=1.0; ///< the end frame of the animation export
  bool m_selected = false; ///< are we exporting selected objects (true) or all objects (false)
  bool m_meshes = true; ///< if true, export meshes
  bool m_meshPoints = true; ///< if true mesh vertices will be exported
  bool m_meshConnects = true; ///< if true face connects and counts will be exported
  bool m_meshNormals = true; ///< if true normal vectors will be exported
  bool m_meshVertexCreases = true; ///< if true vertex creases will be exported
  bool m_meshEdgeCreases = true; ///< if true edge creases will be exported
  bool m_meshUvs = true; ///< if true UV coordinates will be exported
  bool m_meshColours = true; ///< if true colour sets will be exported
  bool m_meshHoles = true; ///< if true polygonal holes will be exported
  bool m_meshUV = false; ///< if true, export a scene hierarchy with all empty prims marked "over", only meshes UV will be filled in.
  bool m_nurbsCurves = true; ///< if true export nurbs curves
  bool m_dynamicAttributes = true; ///< if true export any dynamic attributes found on the nodes we are exporting
  bool m_duplicateInstances = true; ///< if true, instances will be exported as duplicates. As of 23/01/17, nothing will be exported if set to false.
  bool m_mergeTransforms = true; ///< if true, shapes will be merged into their parent transforms in the exported data. If false, the transform and shape will be exported seperately
  bool m_animation = false; ///< if true, animation will be exported.
  bool m_useTimelineRange = false; ///< if true, then the export uses Maya's timeline range.
  bool m_filterSample = false; ///< if true, duplicate sample of attribute will be filtered out
  int m_compactionLevel = 3; ///< by default apply the strongest level of data compaction
  AnimationTranslator* m_animTranslator = 0; ///< the animation translator to help exporting the animation data
  bool m_extensiveAnimationCheck = true; ///< if true, extensive animation check will be performed on transform nodes.
  int m_exportAtWhichTime = 0; ///< controls where the data will be written to: 0 = default time, 1 = earliest time, 2 = current time
  UsdTimeCode m_timeCode = UsdTimeCode::Default();
};

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
