//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
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
#include "test_usdmaya.h"
#include <array>

#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"

#include <maya/MDagModifier.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFileIO.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnTransform.h>
#include <maya/MPointArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnFloatArrayData.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>

using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;

#ifndef USE_AL_DEFAULT
 #define USE_AL_DEFAULT 0
#endif

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
//----------------------------------------------------------------------------------------------------------------------

MObject createNurbStage(bool useSingleWidth=false)
{
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  UsdGeomNurbsCurves nurb = UsdGeomNurbsCurves::Define(stage, SdfPath("/nurb"));

  VtArray<int32_t> curveVertextCounts;
  curveVertextCounts.push_back(5);

  std::array<double, 7> knotsa = {0., 0., 0., 1., 2., 2., 2.};
  VtDoubleArray knots = VtDoubleArray(7);
  memcpy(knots.data(), &knotsa, sizeof(double)*7);

  std::vector<std::array<float, 3>> pointsa = { {-1.5079714f, 44.28195f, 5.781988f},
                            {-1.5784601f, 44.300205f, 5.813314f},
                            {-2.4803247f, 44.201904f, 6.2143235f},
                            {-3.9173129f, 43.33975f, 6.475575f},
                            {-5.2281976f, 42.145287f, 6.6371536f} };
  VtVec3fArray points = VtVec3fArray(5);
  for(uint32_t i = 0 ; i< pointsa.size(); ++i)
  {
    memcpy((void*)&points[i], &pointsa[i], 3*sizeof(float));
  }

  std::vector<std::array<double,2> > rangesa = {{0.,2.}};
  VtVec2dArray ranges = VtVec2dArray(2);

  for(uint32_t i = 0 ; i< rangesa.size(); ++i)
  {
    memcpy((void*)&ranges[i], &rangesa[i], 2*sizeof(double));
  }

  if(useSingleWidth)
  {
    VtFloatArray widths;
    widths.push_back(0.025f);
    nurb.GetWidthsAttr().Set(widths);
  }
  else
  {
    std::array<float, 5> widthValues = {0.025f, 0.025f, 0.025f, 0.025f, 0.001f};
    VtFloatArray widths = VtFloatArray(5);
    memcpy(widths.data(), &widthValues, sizeof(float)*5);
    nurb.GetWidthsAttr().Set(widths);
  }

  nurb.GetCurveVertexCountsAttr().Set(curveVertextCounts);
  nurb.GetKnotsAttr().Set(knots);
  nurb.GetPointsAttr().Set(points);
  nurb.GetRangesAttr().Set(ranges);

  VtArray<GfVec3f> extent(2);
  UsdGeomPointBased::ComputeExtent(points, &extent);
  nurb.GetExtentAttr().Set(extent);

  VtArray<int> order;
  order.push_back(4);
  nurb.GetOrderAttr().Set(order);

  MFnTransform fnx;
  MObject parent = fnx.create();

  UsdPrim prim = nurb.GetPrim();
  AL::usdmaya::fileio::translators::TranslatorManufacture manufacture(nullptr);

  AL::usdmaya::fileio::translators::TranslatorRefPtr translator = manufacture.getTranslatorFromId(AL::usdmaya::fileio::translators::TranslatorManufacture::TranslatorPrefixSchemaType.GetString() + prim.GetTypeName().GetString());
  if (translator)
  {
    MObject createdNode;
    translator->import(prim, parent, createdNode);
    return createdNode;
  }
  else
  {
    return MObject::kNullObj;
  }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that a single width  imported correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_NurbsCurveTranslator, test_width)
{
  MFileIO::newFile(true);
  const bool useSingleWidth = true;
  // Create and write out stage that contains a curve
  MObject nurbObj = createNurbStage(useSingleWidth);

  ASSERT_TRUE(!nurbObj.isNull());

  MFnNurbsCurve nurbs(nurbObj);

  const char* plugName = "width";

  MStatus s2;
  MPlug widthsPlug = nurbs.findPlug(plugName, &s2);
  ASSERT_EQ(s2, MS::kSuccess);

  // test the values came through!
  EXPECT_FLOAT_EQ(widthsPlug.asFloat(), 0.025f);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that multiple widths are imported correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_NurbsCurveTranslator, test_widths)
{
  MFileIO::newFile(true);
  // Create and write out stage that contains a curve
  MObject nurbObj = createNurbStage();
  ASSERT_TRUE(!nurbObj.isNull());
  MFnNurbsCurve nurbs(nurbObj);

  const char* plugName = "width";
  
  MStatus s2;
  MPlug widthsPlug = nurbs.findPlug(plugName, &s2);
  ASSERT_EQ(s2, MS::kSuccess);

  MObject value;
  widthsPlug.getValue(value);
  MFnFloatArrayData floatData;
  floatData.setObject(value);
   
  // test the values came through!
  ASSERT_EQ(floatData.length(), 5u);
  EXPECT_FLOAT_EQ(floatData[0], 0.025f);
  EXPECT_FLOAT_EQ(floatData[1], 0.025f);
  EXPECT_FLOAT_EQ(floatData[2], 0.025f);
  EXPECT_FLOAT_EQ(floatData[3], 0.025f);
  EXPECT_FLOAT_EQ(floatData[4], 0.001f);
}

#if 0
namespace
{
inline SdfPath makeUsdPath(const MDagPath& rootPath, const MDagPath& path)
{
  // if the rootPath is empty, we can just use the entire path
  const uint32_t rootPathLength = rootPath.length();
  if(!rootPathLength)
  {
    std::string fpn = AL::maya::utils::convert(path.fullPathName());
    std::replace_if(fpn.begin(), fpn.end(), [](char c) { return c == '|'; }, '/');
    return SdfPath(fpn);
  }

  // otherwise we need to do a little fiddling.
  MString rootPathString, pathString, newPathString;
  rootPathString = rootPath.fullPathName();
  pathString = path.fullPathName();

  // trim off the root path from the object we are exporting
  newPathString = pathString.substring(rootPathString.length(), pathString.length());

  std::string fpn = AL::maya::utils::convert(newPathString);
  std::replace_if(fpn.begin(), fpn.end(), [](char c) { return c == '|'; }, '/');
  return SdfPath(fpn);
}
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_NurbsCurveTranslator, test_io)
{
  NurbsCurveTranslator::registerType();
  for(int i = 0; i < 100; ++i)
  {
    MFnNurbsCurve fnCurve;

    // choose a random form for the curve
    MFnNurbsCurve::Form forms[] =
    {
      MFnNurbsCurve::kOpen,
      MFnNurbsCurve::kClosed,
      MFnNurbsCurve::kPeriodic
    };
    // TODO Try as I might, I can't get MFnNurbsCurve to generate a periodic curve
    MFnNurbsCurve::Form form = forms[rand() % 2];

    // choose random degree (order is always 1 + degree)
    uint32_t degree = rand() % 10 + 1;
    uint32_t order = degree + 1;

    // generate random number of points (ensuring we have enough CV's to match the chosen curve degree)
    MPointArray points;
    points.setLength((rand() % 20) + order);
    for(uint32_t i = 0; i < points.length(); ++i)
    {
      points[i].x = randDouble();
      points[i].y = randDouble();
      points[i].z = randDouble();
    }

    // make sure the periodic curves have duplicate points at the end
    if(MFnNurbsCurve::kPeriodic == form)
    {
      for(int i = 0; i < degree; ++i)
      {
        points[points.length() - order + i] = points[i];
      }
    }

    // how many segments do we have?
    const uint32_t numCurveSegments = points.length() - degree;

    // the start and end knot values
    float startKnotValue = 0.0f;
    float endKnotValue = float(numCurveSegments);

    // know vectors here are a little weird in maya. In normal maths, if you have 1 cubic curve segment (degree 3),
    // you'll have 1 + 3 + 4 knot values. Maya appears to compute the nurbs curves using forward differencing,
    // so it ignore the first and last knots (hence we need to subtract 2).
    const uint32_t numKnots = numCurveSegments + order + degree - 2;

    // generate random number of points.
    MDoubleArray knots;
    knots.setLength(numKnots);

    // fill central portion of the knot vector
    for(uint32_t i = 0, j = degree - 1; i <= (numCurveSegments); ++i, ++j)
    {
      knots[j] = float(i);
    }

    // periodic curves
    if(form == MFnNurbsCurve::kPeriodic)
    {
      for(int i = 0, j = degree - 1; j >= 0; --j, --i)
      {
        knots[j] = float(i);
      }
      for(int i = numCurveSegments, j = numCurveSegments + degree - 1; j < numKnots; ++j, ++i)
      {
        knots[j] = float(i);
      }
    }
    else
    // clamp curve to end values
    {
      for(int j = degree - 1; j >= 0; --j)
      {
        knots[j] = startKnotValue;
      }
      for(int j = numCurveSegments + degree - 1; j < numKnots; ++j)
      {
        knots[j] = endKnotValue;
      }
    }

    MFnTransform fnx;
    MFnNurbsCurve fn;

    MDagPath path, path2;
    MObject xform = fnx.create();
    MObject curve = fn.create(points, knots, degree, form, false, false, xform);
    fn.getPath(path);
    MObject xform2 = fnx.create();
    fnx.getPath(path2);

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    // export curve
    SdfPath usdpath("/curvey");
    ExporterParams options;
    UsdPrim prim = NurbsCurveTranslator::exportObject(stage, path, usdpath, options);
    EXPECT_TRUE(prim);

    std::string s;
    stage->GetRootLayer()->ExportToString(&s);

    // TODO This interface is miserable. Make less miserable.
    ImporterParams params;
    NurbsCurveTranslator xlator;
    EXPECT_TRUE(MObject::kNullObj != xlator.createNode(prim, xform2, "nurbsCurve", params));
    path2.extendToShape();

    const char* const attributeNames[] = {
        "visibility",
        "intermediateObject",
        "tweak",
        "relativeTweak",
        "controlPoints",
        "weights",
        "lineWidth",
        "worldSpace",
        "worldNormal",
        "form",
        "degree",
        "spans",
        "editPoints",
        "inPlace",
        "dispCV",
        "dispEP",
        "dispHull",
        "dispCurveEndPoints",
        "dispGeometry",
        "tweakSize",
        "minMaxValue"
    };
    const uint32_t numAttrs = sizeof(attributeNames) / sizeof(const char*);

    // now make sure the imported node matches the one we started with
    compareNodes(curve, path2.node(), attributeNames, numAttrs);

    //
    MDagModifier mod;
    mod.deleteNode(path2.node());
    mod.deleteNode(xform2);
    mod.deleteNode(curve);
    mod.deleteNode(xform);
    mod.doIt();
  }
}
#endif
