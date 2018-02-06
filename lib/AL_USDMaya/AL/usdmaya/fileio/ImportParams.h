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
#include "maya/MDagPath.h"
#include "maya/MString.h"
#include "maya/MStringArray.h"

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "AL/usd/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE


namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  parameters for the importer
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct ImporterParams
{
  MDagPath m_parentPath; ///< the parent transform under which the USD file will be imported
  MString m_fileName; ///< the name of the file to import
  bool m_meshes = true; ///< true to import mesh geometry, false to ignore mesh geometry on import
  bool m_animations = true; ///< true to import animation data, false to ignore animation data import
  bool m_nurbsCurves = true; ///< true to import nurbs curves, false to ignore nurbs curves on import
  bool m_dynamicAttributes = true; ///< if true, attributes in the USD file marked as 'custom' will be imported as dynamic attributes.
  bool m_stageUnloaded = true; ///< if true, the USD stage will be opened with the UsdStage::LoadNone flag. If false the stage will be loaded with the UsdStage::LoadAll flag
  SdfLayerRefPtr m_rootLayer; ///< \todo  Remove?
  SdfLayerRefPtr m_sessionLayer; ///< \todo  Remove?
};

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
