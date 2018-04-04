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
#include "AL/usdmaya/fileio/Import.h"
#include "AL/usdmaya/fileio/ImportTranslator.h"

#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportTranslator::reader(const MFileObject& file, const AL::maya::utils::OptionsParser& options, FileAccessMode mode)
{
  MString parentPath = options.getString(kParentPath);
  m_params.m_parentPath = MDagPath();
  if(parentPath.length())
  {
    MSelectionList sl, sl2;
    MGlobal::getActiveSelectionList(sl);
    MGlobal::selectByName(parentPath, MGlobal::kReplaceList);
    MGlobal::getActiveSelectionList(sl2);
    MGlobal::setActiveSelectionList(sl);
    if(sl2.length())
    {
      sl2.getDagPath(0, m_params.m_parentPath);
    }
  }

  m_params.m_fileName = file.fullName();
  m_params.m_meshes = options.getBool(kMeshes);
  m_params.m_nurbsCurves = options.getBool(kNurbsCurves);
  m_params.m_animations = options.getBool(kAnimations);
  m_params.m_animations = options.getBool(kAnimations);

  Import importer(m_params);

  return importer ? MS::kSuccess : MS::kFailure; // done!
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
