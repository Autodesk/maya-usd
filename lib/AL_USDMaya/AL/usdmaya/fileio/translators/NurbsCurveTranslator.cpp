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
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/translators/NurbsCurveTranslator.h"

#include "maya/MDoubleArray.h"
#include "maya/MDagPath.h"
#include "maya/MFnNurbsCurve.h"
#include "maya/MGlobal.h"
#include "maya/MPointArray.h"

#include "pxr/usd/usdGeom/nurbsCurves.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
void floatToDouble(double* output, const float* const input, size_t count);
void doubleToFloat(float* output, const double* const input, size_t count);
void convert3DFloatArrayTo4DDoubleArray(const float* const input, double* const output, size_t count)
{
  for(size_t i = 0, j = 0, n = count * 4; i != n; i += 4, j += 3)
  {
    output[i] = input[j];
    output[i + 1] = input[j + 1];
    output[i + 2] = input[j + 2];
    output[i + 3] = 1.0;
  }
}

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

  const UsdGeomNurbsCurves xformSchema(from);

  UsdAttribute usdKnots = xformSchema.GetKnotsAttr();
  VtArray<double> dataKnots;
  usdKnots.Get(&dataKnots);
  if(dataKnots.size() == 0)
    return MObject::kNullObj;

  UsdAttribute usdPoints = xformSchema.GetPointsAttr();
  VtArray<GfVec3f> dataPoints;
  usdPoints.Get(&dataPoints);
  if(dataPoints.size() == 0)
    return MObject::kNullObj;

  UsdAttribute usdOrder = xformSchema.GetOrderAttr();
  VtArray<int32_t> dataOrders;
  usdOrder.Get(&dataOrders);
  if(dataOrders.size() == 0)
    return MObject::kNullObj;

  UsdAttribute usdCurveVertexCounts = xformSchema.GetCurveVertexCountsAttr();
  VtArray<int32_t> dataCurveVertexCounts;
  usdCurveVertexCounts.Get(&dataCurveVertexCounts);
  if(dataCurveVertexCounts.size() == 0)
    return MObject::kNullObj;

  MPointArray controlVertices;
  MDoubleArray knotSequences;

  size_t currentPointIndex = 0;
  size_t currentKnotIndex = 0;

  MFnNurbsCurve fnCurve;

  for(size_t i = 0, ncurves = dataCurveVertexCounts.size(); i < ncurves; ++i)
  {
    const int32_t numPoints = dataCurveVertexCounts[i];
    controlVertices.setLength(numPoints);

    const int32_t numKnots = numPoints + dataOrders[i] - 2;
    knotSequences.setLength(numKnots);

    const float* pstart = (const float*)&dataPoints[currentPointIndex];
    const double* kstart = &dataKnots[currentKnotIndex];
    memcpy(&knotSequences[0], kstart, sizeof(double) * numKnots);

    currentPointIndex += numPoints;
    currentKnotIndex += numKnots;

    convert3DFloatArrayTo4DDoubleArray(pstart, (double*)&controlVertices[0], numPoints);
    fnCurve.create(controlVertices, knotSequences, dataOrders[i] - 1, MFnNurbsCurve::kOpen, false, false, parent);
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
  UsdGeomNurbsCurves nurbs = UsdGeomNurbsCurves::Define(stage, usdPath);

  MFnNurbsCurve fnCurve(path);

  MPointArray controlVertices;
  MDoubleArray knotSequences;
  fnCurve.getCVs(controlVertices);
  fnCurve.getKnots(knotSequences);

  VtArray<int32_t> dataCurveVertexCounts;
  VtArray<GfVec3f> dataPoints;
  VtArray<double> dataKnots;
  VtArray<int32_t> dataOrders;
  VtArray<GfVec2d> dataRanges;

  dataCurveVertexCounts.push_back(controlVertices.length());
  dataOrders.push_back(fnCurve.degree() + 1);

  double start, end;
  fnCurve.getKnotDomain(start, end);
  dataRanges.push_back(GfVec2d(start, end));

  dataPoints.resize(controlVertices.length());
  for(uint32_t i = 0; i < controlVertices.length(); ++i)
  {
    dataPoints[i][0] = controlVertices[i].x;
    dataPoints[i][1] = controlVertices[i].y;
    dataPoints[i][2] = controlVertices[i].z;
  }

  dataKnots.resize(knotSequences.length());
  for(uint32_t i = 0; i < knotSequences.length(); ++i)
  {
    dataKnots[i] = knotSequences[i];
  }

  nurbs.GetCurveVertexCountsAttr().Set(dataCurveVertexCounts);
  nurbs.GetPointsAttr().Set(dataPoints);
  nurbs.GetOrderAttr().Set(dataOrders);
  nurbs.GetRangesAttr().Set(dataRanges);
  nurbs.GetKnotsAttr().Set(dataKnots);

  return nurbs.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
