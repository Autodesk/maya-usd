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
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"

#include "maya/MObject.h"

#include "pxr/usd/usd/stage.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class to transfer mesh data between Usd <--> Maya
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class MeshTranslator
  : public DagNodeTranslator
{
public:

  /// \brief  static type registration
  /// \return MS::kSuccess if ok
  static MStatus registerType();

  /// \brief  Creates a new maya node of the given type and set attributes based on input prim
  /// \param  from the UsdPrim to copy the data from
  /// \param  parent the parent Dag node to parent the newly created object under
  /// \param  nodeType the maya node type to create
  /// \param  params the importer params that determines what will be imported
  /// \return the newly created node
  MObject createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params) override;

  /// \brief  Copies data from the maya node onto the usd primitive
  /// \param  from the maya node to copy the data from
  /// \param  to the USD prim to copy the attributes to
  /// \param  params the exporter params to determine what should be exported
  /// \return MS::kSuccess if ok
  static MStatus copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params);

  /// \brief  Copies vertex data from the maya mesh onto the usd geometry mesh
  /// \param  fnMesh the maya mesh to copy the data from
  /// \param  pointsAttr the USD geometry mesh points attribute to copy to
  /// \param  time the timecode to use when setting the data
  static void copyVertexData(const MFnMesh& fnMesh, const UsdAttribute& pointsAttr, UsdTimeCode time = UsdTimeCode::Default());

  /// \brief  exports a mesh to the USD file and returns the created prim
  /// \param  stage  the stage in which to create the prim
  /// \param  mayaPath  the path to the maya mesh to export
  /// \param  usdPath  the usd path where the prim should be created
  /// \param  params  the export options
  /// \return the newly created usd prim
  static UsdPrim exportObject(UsdStageRefPtr stage, MDagPath mayaPath, const SdfPath& usdPath, const ExporterParams& params);

  /// \brief  exports only UV of a mesh to the USD file and returns the overridden prim
  /// \param  stage  the stage in which to override the prim
  /// \param  mayaPath  the path to the maya curve to export
  /// \param  usdPath  the usd path where the prim should be created
  /// \param  params  the export options
  /// \return the overridden usd prim
  static UsdPrim exportUV(UsdStageRefPtr stage, MDagPath mayaPath, const SdfPath& usdPath, const ExporterParams& params);

  /// \brief  import the dynamic attribute import that we are expected to see some extra subdiv animal logic only data
  ///         exported with our meshes.
  /// \param  usdAttr the attribute to test
  /// \return true if prefixed with 'alusd_'
  bool attributeHandled(const UsdAttribute& usdAttr) override;
};

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
