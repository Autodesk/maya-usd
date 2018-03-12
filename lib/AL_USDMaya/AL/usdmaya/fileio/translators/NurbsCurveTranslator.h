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
/// \brief  A utility class to transfer nurbs curves between maya and usd.
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class NurbsCurveTranslator
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

  /// \brief  exports a nurbs curve to the USD file and returns the created prim
  /// \param  stage  the stage in which to create the prim
  /// \param  mayaPath  the path to the maya curve to export
  /// \param  usdPath  the usd path where the prim should be created
  /// \param  params  the export options
  /// \return the newly created usd prim
  static UsdPrim exportObject(UsdStageRefPtr stage, MDagPath mayaPath, const SdfPath& usdPath, const ExporterParams& params);
};

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
