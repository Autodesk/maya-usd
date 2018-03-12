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
#include "AL/usdmaya/fileio/Export.h"
#include "AL/usdmaya/fileio/ExportTranslator.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"

#include "maya/MAnimControl.h"
#include "maya/MDagPath.h"
#include "maya/MGlobal.h"
#include "maya/MItDag.h"
#include "maya/MSelectionList.h"

#include <unordered_set>

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportTranslator::writer(const MFileObject& file, const AL::maya::utils::OptionsParser& options, FileAccessMode mode)
{
  static const std::unordered_set<std::string> ignoredNodes
  {
    "persp",
    "front",
    "top",
    "side"
  };

  ExporterParams params;
  params.m_dynamicAttributes = options.getBool(kDynamicAttributes);
  params.m_duplicateInstances = options.getBool(kDuplicateInstances);
  params.m_meshes = options.getBool(kMeshes);
  params.m_nurbsCurves = options.getBool(kNurbsCurves);
  params.m_useAnimalSchema = options.getBool(kUseAnimalSchema);
  params.m_mergeTransforms = options.getBool(kMergeTransforms);
  params.m_fileName = file.fullName();
  params.m_selected = mode == MPxFileTranslator::kExportActiveAccessMode;
  params.m_animation = options.getBool(kAnimation);
  if(params.m_animation)
  {
    if(options.getBool(kUseTimelineRange))
    {
      params.m_minFrame = MAnimControl::minTime().value();
      params.m_maxFrame = MAnimControl::maxTime().value();
    }
    else
    {
      params.m_minFrame = options.getFloat(kFrameMin);
      params.m_maxFrame = options.getFloat(kFrameMax);
    }
    params.m_animTranslator = new AnimationTranslator;
  }
  params.m_filterSample = options.getBool(kFilterSample);
  if(params.m_selected)
  {
    MGlobal::getActiveSelectionList(params.m_nodes);
  }
  else
  {
    MDagPath path;
    MItDag it(MItDag::kDepthFirst, MFn::kTransform);
    while(!it.isDone())
    {
      it.getPath(path);
      const MString name = path.partialPathName();
      const std::string s(name.asChar(), name.length());
      if(ignoredNodes.find(s) == ignoredNodes.end())
      {
        params.m_nodes.add(path);
      }
      it.prune();
      it.next();
    }
  }

  Export exporter(params);

  if(params.m_animTranslator)
  {
    delete params.m_animTranslator;
  }

  // import your data

  return MS::kSuccess; // done!
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
