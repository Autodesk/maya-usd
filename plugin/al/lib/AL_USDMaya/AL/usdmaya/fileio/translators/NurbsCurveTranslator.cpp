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
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/utils/NurbsCurveUtils.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/translators/NurbsCurveTranslator.h"
#include "AL/maya/utils/NodeHelper.h"

#include "maya/MDoubleArray.h"
#include "maya/MFnDoubleArrayData.h"
#include "maya/MDagPath.h"
#include "maya/MFnNurbsCurve.h"
#include "maya/MGlobal.h"
#include "maya/MPointArray.h"
#include "maya/MFnNumericAttribute.h"
#include "maya/MFnFloatArrayData.h"

#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/base/tf/debug.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurveTranslator::registerType()
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NurbsCurveTranslator::createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params)
{
  if(!params.m_nurbsCurves)
    return MObject::kNullObj;

  MFnNurbsCurve fnCurve;
  const UsdGeomNurbsCurves usdCurves(from);

  TfToken mtVal;
  bool parentUnmerged = false;
  if (from.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &mtVal))
  {
    parentUnmerged = (mtVal == AL::usdmaya::Metadata::unmerged);
  }

  if (!AL::usdmaya::utils::createMayaCurves(fnCurve, parent, usdCurves, parentUnmerged))
  {
    return MObject::kNullObj;
  }

  MObject object = fnCurve.object();
  AL_MAYA_CHECK_ERROR_RETURN_NULL_MOBJECT(DagNodeTranslator::copyAttributes(from, object, params), "Failed to copy attributes");

  return object;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim NurbsCurveTranslator::exportObject(UsdStageRefPtr stage, MDagPath path, const SdfPath& usdPath, const ExporterParams& params)
{
  if(!params.m_nurbsCurves)
    return UsdPrim();

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::Starting to export Nurbs for path '%s'\n", usdPath.GetText());

  UsdGeomNurbsCurves nurbs = UsdGeomNurbsCurves::Define(stage, usdPath);
  MFnNurbsCurve fnCurve(path);

  AL::usdmaya::utils::copyPoints(fnCurve, nurbs.GetPointsAttr(), params.m_timeCode);
  AL::usdmaya::utils::copyCurveVertexCounts(fnCurve, nurbs.GetCurveVertexCountsAttr(), params.m_timeCode);
  AL::usdmaya::utils::copyKnots(fnCurve, nurbs.GetKnotsAttr(), params.m_timeCode);
  AL::usdmaya::utils::copyRanges(fnCurve, nurbs.GetRangesAttr(), params.m_timeCode);
  AL::usdmaya::utils::copyOrder(fnCurve, nurbs.GetOrderAttr(), params.m_timeCode);

  MObject widthObj;
  MPlug widthPlug;
  MFnDoubleArrayData widthArray;

  if (!AL::usdmaya::utils::getMayaCurveWidth(fnCurve, widthObj, widthPlug, widthArray))
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::No width/s attribute found for path '%s' \n", usdPath.GetText());
  }

  if(!widthObj.isNull() && !widthPlug.isNull())
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::Exporting width/s for path '%s' \n", usdPath.GetText());
    AL::usdmaya::utils::copyWidths(widthObj, widthPlug, widthArray, nurbs.GetWidthsAttr());
  }

  return nurbs.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
