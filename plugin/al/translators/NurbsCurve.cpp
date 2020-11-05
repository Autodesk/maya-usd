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

#include "NurbsCurve.h"

#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/NurbsCurveUtils.h"
#include "CommonTranslatorOptions.h"

#include <pxr/usd/usdGeom/nurbsCurves.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDoubleArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MNodeClass.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

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
    MNodeClass        fn("transform");
    MStatus           status;

    m_visible = fn.attribute("v", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurve::import prim=%s\n", prim.GetPath().GetText());

    MFnNurbsCurve            fnCurve;
    const UsdGeomNurbsCurves usdCurves(prim);

    TfToken mtVal;
    bool    parentUnmerged = false;
    if (prim.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &mtVal)) {
        parentUnmerged = (mtVal == AL::usdmaya::Metadata::unmerged);
    }

    if (!AL::usdmaya::utils::createMayaCurves(fnCurve, parent, usdCurves, parentUnmerged)) {
        return MStatus::kFailure;
    }

    // replicate of DagNodeTranslator::copyAttributes
    MObject object = fnCurve.object();
    createdObj = object;
    const UsdGeomXform xformSchema(prim);
    DgNodeTranslator::copyBool(object, m_visible, xformSchema.GetVisibilityAttr());
    // pick up any additional attributes attached to the mesh node (these will be added alongside
    // the transform attributes)
    const std::vector<UsdAttribute> attributes = prim.GetAttributes();
    for (size_t i = 0; i < attributes.size(); ++i) {
        if (attributes[i].IsAuthored() && attributes[i].HasValue() && attributes[i].IsCustom()) {
            DgNodeTranslator::addDynamicAttribute(object, attributes[i]);
        }
    }

    TranslatorContextPtr ctx = context();
    if (ctx) {
        ctx->addExcludedGeometry(prim.GetPath());
        ctx->insertItem(prim, createdObj);
    }
    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim NurbsCurve::exportObject(
    UsdStageRefPtr        stage,
    MDagPath              dagPath,
    const SdfPath&        usdPath,
    const ExporterParams& params)
{
    if (!params.getBool(GeometryExportOptions::kNurbsCurves))
        return UsdPrim();

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorContext::Starting to export Nurbs for path '%s'\n", usdPath.GetText());

    UsdGeomNurbsCurves nurbs = UsdGeomNurbsCurves::Define(stage, usdPath);
    MFnNurbsCurve      fnCurve(dagPath);
    writeEdits(nurbs, fnCurve, true);

    // pick up any additional attributes attached to the curve node (these will be added alongside
    // the transform attributes)
    if (params.m_dynamicAttributes) {
        UsdPrim prim = nurbs.GetPrim();
        DgNodeTranslator::copyDynamicAttributes(dagPath.node(), prim);
    }

    if (params.getBool(GeometryExportOptions::kMeshPointsAsPref)) {
        AL::usdmaya::utils::copyNurbsCurveBindPoseData(fnCurve, nurbs, params.m_timeCode);
    }

    if (params.getBool(GeometryExportOptions::kMeshExtents)) {
        AL::usdmaya::utils::copyExtent(fnCurve, nurbs.GetExtentAttr(), params.m_timeCode);
    }

    return nurbs.GetPrim();
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
MStatus NurbsCurve::update(const UsdPrim& prim) { return MStatus::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus NurbsCurve::preTearDown(UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("NurbsCurveTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());
    TranslatorBase::preTearDown(prim);

    /* TODO
     * This block was put in since writeEdits modifies USD and thus triggers the OnObjectsChanged
     * callback which will then tearDown the this Mesh prim. The writeEdits method will then
     * continue attempting to copy maya mesh data to USD but will end up crashing since the maya
     * mesh has now been removed by the tearDown.
     *
     * I have tried turning off the TfNotice but I get the 'Detected usd threading violation.
     * Concurrent changes to layer(s) composed' error.
     *
     * This crash and error seems to be happening mainly when switching out a variant that contains
     * a Mesh, and that Mesh has been force translated into Maya.
     */
    TfNotice::Block block;
    if (!prim.IsValid()) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("NurbsCurve writeEdits prim invalid\n");
        return MStatus::kFailure;
    }
    // Write the overrides back to the path it was imported at
    MObjectHandle obj;
    context()->getMObject(prim, obj, MFn::kInvalid);

    if (obj.isValid()) {
        UsdGeomNurbsCurves nurbsCurves(prim);

        MFnDagNode fn(obj.object());
        MDagPath   path;
        fn.getPath(path);
        MStatus       status;
        MFnNurbsCurve fnCurve(path, &status);
        AL_MAYA_CHECK_ERROR2(
            status, MString("unable to attach function set to nurbs curve ") + path.fullPathName());

        if (status) {
            writeEdits(nurbsCurves, fnCurve, false);
        }
    }

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void NurbsCurve::writeEdits(
    UsdGeomNurbsCurves& nurbsCurvesPrim,
    MFnNurbsCurve&      fnCurve,
    bool                writeAll)
{
    uint32_t diff_curves = AL::usdmaya::utils::kAllNurbsCurveComponents;
    bool     performDiff = !writeAll;
    if (performDiff) {
        diff_curves = AL::usdmaya::utils::diffNurbsCurve(
            nurbsCurvesPrim,
            fnCurve,
            UsdTimeCode::Default(),
            AL::usdmaya::utils::kAllNurbsCurveComponents);
    }

    if (diff_curves & AL::usdmaya::utils::kCurvePoints) {
        AL::usdmaya::utils::copyPoints(fnCurve, nurbsCurvesPrim.GetPointsAttr());
    }
    if (diff_curves & AL::usdmaya::utils::kCurveExtent) {
        AL::usdmaya::utils::copyExtent(fnCurve, nurbsCurvesPrim.GetExtentAttr());
    }
    if (diff_curves & AL::usdmaya::utils::kCurveVertexCounts) {
        AL::usdmaya::utils::copyCurveVertexCounts(
            fnCurve, nurbsCurvesPrim.GetCurveVertexCountsAttr());
    }
    if (diff_curves & AL::usdmaya::utils::kKnots) {
        UsdAttribute knotsAttr = nurbsCurvesPrim.GetKnotsAttr();
        AL::usdmaya::utils::copyKnots(fnCurve, knotsAttr);
    }
    if (diff_curves & AL::usdmaya::utils::kRanges) {
        UsdAttribute rangeAttr = nurbsCurvesPrim.GetRangesAttr();
        AL::usdmaya::utils::copyRanges(fnCurve, rangeAttr);
    }
    if (diff_curves & AL::usdmaya::utils::kOrder) {
        AL::usdmaya::utils::copyOrder(fnCurve, nurbsCurvesPrim.GetOrderAttr());
    }
    if (diff_curves & AL::usdmaya::utils::kWidths) {
        /*TODO: Move into AL internal ExtraData translator code as this Width/Widths attribute
          follows internal render conventions*/
        MObject widthObj;
        MPlug   widthPlug;

        if (!AL::usdmaya::utils::getMayaCurveWidth(fnCurve, widthObj, widthPlug)) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::No width/s attribute found for path '%s' \n",
                    nurbsCurvesPrim.GetPath().GetText());
        }
        if (!widthObj.isNull() && !widthPlug.isNull()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::Exporting width/s for path '%s' \n",
                    nurbsCurvesPrim.GetPath().GetText());
        }

        if (widthObj.apiType() != MFn::kInvalid) {
            if (widthObj.apiType() == MFn::kDoubleArrayData) {
                MFnDoubleArrayData widthArray;
                widthArray.setObject(widthObj);
                AL::usdmaya::utils::copyWidths(
                    widthObj, widthPlug, widthArray, nurbsCurvesPrim.GetWidthsAttr());
            } else if (widthObj.apiType() == MFn::kFloatArrayData) {
                MFnFloatArrayData widthArray;
                widthArray.setObject(widthObj);
                AL::usdmaya::utils::copyWidths(
                    widthObj, widthPlug, widthArray, nurbsCurvesPrim.GetWidthsAttr());
            }
        } else if (
            MFnNumericAttribute(widthPlug.attribute()).unitType() == MFnNumericData::kDouble
            || MFnNumericAttribute(widthPlug.attribute()).unitType() == MFnNumericData::kFloat) {
            VtArray<float> dataWidths;
            dataWidths.push_back(widthPlug.asFloat());
            nurbsCurvesPrim.GetWidthsAttr().Set(dataWidths);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
