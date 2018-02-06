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

namespace AL {
namespace usdmaya {
namespace fileio {

#ifndef USE_AL_DEFAULT
 #define USE_AL_DEFAULT 0
#endif

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
  bool m_meshUV = false; ///< if true, export a scene hierarchy with all empty prims marked "over", only meshes UV will be filled in.
  bool m_leftHandedUV = false; ///< if true, UV indices retrieved from Maya will be adjusted to left-handed orientation, it only works with m_meshUV.
  bool m_nurbsCurves = true; ///< if true export nurbs curves
  bool m_dynamicAttributes = true; ///< if true export any dynamic attributes found on the nodes we are exporting
  bool m_duplicateInstances = true; ///< if true, instances will be exported as duplicates. As of 23/01/17, nothing will be exported if set to false.
  bool m_mergeTransforms = true; ///< if true, shapes will be merged into their parent transforms in the exported data. If false, the transform and shape will be exported seperately
  bool m_animation = false; ///< if true, animation will be exported.
  bool m_useTimelineRange = false; ///< if true, then the export uses Maya's timeline range.
  bool m_filterSample = false; ///< if true, duplicate sample of attribute will be filtered out
  bool m_useAnimalSchema = (USE_AL_DEFAULT) ? true : false; ///< if true, the data exported will be designed to fit with Animal Logics internal needs. If false, the original pxr schema will be used.
  AnimationTranslator* m_animTranslator = 0; ///< the animation translator to help exporting the animation data
};

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
