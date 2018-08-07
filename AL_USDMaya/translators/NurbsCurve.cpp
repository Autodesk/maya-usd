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

#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usdGeom/xform.h"

#include "maya/MDoubleArray.h"
#include "maya/MFnNurbsCurve.h"
#include "maya/MPointArray.h"
#include "maya/MStatus.h"
#include "maya/MNodeClass.h"

#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/NurbsCurveUtils.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/Metadata.h"

#include "AL/maya/utils/NodeHelper.h"

#include "NurbsCurve.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
MObject NurbsCurve::m_visible = MObject::kNullObj;

AL_USDMAYA_DEFINE_TRANSLATOR(NurbsCurve, PXR_NS::UsdGeomNurbsCurves)

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::initialize()
{
  const char* const errorString = "Unable to extract attribute for NurbsCurve";
  MNodeClass fn("transform");
  MStatus status;

  m_visible = fn.attribute("v", &status);
  AL_MAYA_CHECK_ERROR(status, errorString);

  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::import(const UsdPrim& prim, MObject& parent)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurve::import prim=%s\n", prim.GetPath().GetText());

  MFnNurbsCurve fnCurve;
  const UsdGeomNurbsCurves usdCurves(prim);

  TfToken mtVal;
  bool parentUnmerged = false;
  if (prim.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &mtVal))
  {
    parentUnmerged = (mtVal == AL::usdmaya::Metadata::unmerged);
  }

  if (!AL::usdmaya::utils::createMayaCurves(fnCurve, parent, usdCurves, parentUnmerged))
  {
    return MStatus::kFailure;
  }

  // replicate of DagNodeTranslator::copyAttributes
  MObject object = fnCurve.object();
  const UsdGeomXform xformSchema(prim);
  DgNodeTranslator::copyBool(object, m_visible, xformSchema.GetVisibilityAttr());
  // pick up any additional attributes attached to the mesh node (these will be added alongside the transform attributes)
  const std::vector<UsdAttribute> attributes = prim.GetAttributes();
  for(size_t i = 0; i < attributes.size(); ++i)
  {
    if(attributes[i].IsAuthored() && attributes[i].HasValue() && attributes[i].IsCustom())
    {
      DgNodeTranslator::addDynamicAttribute(object, attributes[i]);
    }
  }

  context()->addExcludedGeometry(prim.GetPath());
  MFnDagNode mayaNode(parent);
  MDagPath mayaDagPath;
  mayaNode.getPath(mayaDagPath);
  context()->insertItem(prim, parent);

  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::tearDown(const SdfPath& path)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurveTranslator::tearDown prim=%s\n", path.GetText());

  context()->removeItems(path);
  context()->removeExcludedGeometry(path);
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::update(const UsdPrim& prim)
{
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::preTearDown(UsdPrim& prim)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurveTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());
  TranslatorBase::preTearDown(prim);

  /* TODO
   * This block was put in since writeEdits modifies USD and thus triggers the OnObjectsChanged callback which will then tearDown
   * the this Mesh prim. The writeEdits method will then continue attempting to copy maya mesh data to USD but will end up crashing
   * since the maya mesh has now been removed by the tearDown.
   *
   * I have tried turning off the TfNotice but I get the 'Detected usd threading violation. Concurrent changes to layer(s) composed'
   * error.
   *
   * This crash and error seems to be happening mainly when switching out a variant that contains a Mesh, and that Mesh has been
   * force translated into Maya.
   */
  TfNotice::Block block;
  writeEdits(prim);
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void NurbsCurve::writeEdits(UsdPrim& prim)
{
  if(!prim.IsValid())
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurve writeEdits prim invalid\n");
    return;
  }
  // Write the overrides back to the path it was imported at
  MObjectHandle obj;
  context()->getMObject(prim, obj, MFn::kInvalid);

  if(obj.isValid())
  {
    UsdGeomNurbsCurves nurbsCurves(prim);

    MFnDagNode fn(obj.object());
    MDagPath path;
    fn.getPath(path);
    MStatus status;
    MFnNurbsCurve fnCurve(path, &status);
    AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to nurbs curve ") + path.fullPathName());

    if (status)
    {
      const uint32_t diff_curves = AL::usdmaya::utils::diffNurbsCurve(nurbsCurves, fnCurve, UsdTimeCode::Default(),
                                                                      AL::usdmaya::utils::kAllNurbsCurveComponents);
      if (diff_curves & AL::usdmaya::utils::kCurvePoints)
      {
        AL::usdmaya::utils::copyPoints(fnCurve, nurbsCurves.GetPointsAttr());
      }
      if (diff_curves & AL::usdmaya::utils::kCurveVertexCounts)
      {
        AL::usdmaya::utils::copyCurveVertexCounts(fnCurve, nurbsCurves.GetCurveVertexCountsAttr());
      }
      if (diff_curves & AL::usdmaya::utils::kKnots)
      {
        UsdAttribute knotsAttr = nurbsCurves.GetKnotsAttr();
        AL::usdmaya::utils::copyKnots(fnCurve, knotsAttr);
      }
      if (diff_curves & AL::usdmaya::utils::kRanges)
      {
        UsdAttribute rangeAttr = nurbsCurves.GetRangesAttr();
        AL::usdmaya::utils::copyRanges(fnCurve, rangeAttr);
      }
      if (diff_curves & AL::usdmaya::utils::kOrder)
      {
        AL::usdmaya::utils::copyOrder(fnCurve, nurbsCurves.GetOrderAttr());
      }
      if (diff_curves & AL::usdmaya::utils::kWidths)
      {
        MObject widthObj;
        MPlug widthPlug;
        MFnDoubleArrayData widthArray;
        AL::usdmaya::utils::getMayaCurveWidth(fnCurve, widthObj, widthPlug, widthArray);
        AL::usdmaya::utils::copyWidths(widthObj, widthPlug, widthArray, nurbsCurves.GetWidthsAttr());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------

